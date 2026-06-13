#include <glog/logging.h>
#include "mw_base/now.h"
#include "ros/ros.h"

#include "monitor/connection_monitor.h"
#include "shm_probe/shm_probe.h"
#include "mw_shm/shm_transport/shm_transport_config.h"
#include "utils/netlink_helper.h"
#include "utils/proc_stat.h"
#include "utils/ros_utils.h"

namespace mw {
namespace system_stats {

int32_t ConnectionMonitor::Start() {
  thread_ = std::make_unique<std::thread>(
      std::bind(&ConnectionMonitor::collect_connections, this));
  thread_->detach();
  return 0;
}

int32_t ConnectionMonitor::Stop() {
  stop_ = true;
  return 0;
}

void ConnectionMonitor::collect_connections() {
  while (!stop_) {
    ros::V_string local_nodes;
    if (!RosUtils::GetAllLocalNodes(local_nodes)) {
      return;
    }

    std::vector<ros_conn_info> cur_conns;
    for (const auto &node : local_nodes) {
      std::vector<RosUtils::RosConnection> node_conns;

      if (node.find("/record") != std::string::npos) {
        continue;
      }

      if (node.find("/rostopic") != std::string::npos) {
        continue;
      }

      if (!RosUtils::GetNodeConnInfo(node, node_conns)) {
        continue;
      }

      for (auto &one_ros_conn : node_conns) {
        if (filter_connection(one_ros_conn)) {
          continue;
        }

        ros_conn_info ci{};
        ci.node = node;
        ci.topic = one_ros_conn.topic_name;
        ci.direction = one_ros_conn.direction;
        ci.dest_uri = one_ros_conn.dest_uri;
        ci.transport_type = one_ros_conn.transport_type;
        ci.local_port = one_ros_conn.local_port;
        ci.peer_host = one_ros_conn.peer_host;
        ci.peer_port = one_ros_conn.peer_port;
        cur_conns.push_back(ci);
      }
    }

    if (!cur_conns.empty()) {
      std::lock_guard<std::mutex> c_guard{lock_};
      ros_conns_ = cur_conns;
    }

    std::this_thread::sleep_for(std::chrono::seconds(10));
  }

  return;
}

bool ConnectionMonitor::filter_connection(const RosUtils::RosConnection &conn) {
  if (conn.topic_name == "/rosout") {
    return true;
  }

  if (conn.transport_type == "INTRAPROCESS") {
    return true;
  }

  return false;
}

void ConnectionMonitor::appendConnInfo(
    int local_port, const std::string &peer_host, int peer_port,
    int64_t cur_time_ns, std::map<conn_key, conn_stats> *all_conns_cur,
    mw::proto::ConnInfo *one_sock_msg) {
  auto netlink = NetLinkTcp::Instance();
  tcpstat ts;
  if (!netlink->GetOneSocket(local_port, peer_host, peer_port, ts)) {
    return;
  }

  conn_stats cs{};
  cs.local_port = local_port;
  cs.peer_host = peer_host;
  cs.peer_port = peer_port;
  one_sock_msg->set_mss(ts.mss);
  one_sock_msg->set_rtt(ts.rtt);

  one_sock_msg->set_snd_kb(ts.bytes_acked / 1024);
  one_sock_msg->set_rcv_kb(ts.bytes_received / 1024);
  one_sock_msg->set_cwnd(ts.cwnd);
  one_sock_msg->set_un_acked(ts.unacked);

  sockstat *ss = &(ts.ss);
  one_sock_msg->set_rcv_queue(ss->rq);
  one_sock_msg->set_snd_queue(ss->wq);

  tcpmem *tm = &(ts.meminfo);
  one_sock_msg->set_snd_buf(tm->snd_buf);
  one_sock_msg->set_rcv_buf(tm->rcv_buf);
  one_sock_msg->set_rmem(tm->rcv_mem);
  one_sock_msg->set_wmem(tm->snd_mem);
  one_sock_msg->set_drops(tm->drops);

  cs.snd_bytes = ts.bytes_acked;
  cs.rcv_bytes = ts.bytes_received;
  cs.rcv_bytes = ts.bytes_received;
  cs.retrans_bytes = ts.retrans_bytes;
  cs.retrans = ts.retrans;
  cs.retrans_total = ts.retrans_total;
  cs.update_ns = cur_time_ns;

  auto key = std::make_tuple(cs.local_port, cs.peer_host, cs.peer_port);
  (*all_conns_cur)[key] = cs;

  auto old_conn_iter = all_conns_.find(key);
  if (old_conn_iter != all_conns_.end()) {
    auto old_conn = old_conn_iter->second;

    double time_delta =
        static_cast<double>((cs.update_ns - old_conn.update_ns) / 1e9);
    if (time_delta > 0) {
      one_sock_msg->set_snd_speed(
          static_cast<float>(cs.snd_bytes - old_conn.snd_bytes) / 1024 /
          time_delta);
      one_sock_msg->set_rcv_speed(
          static_cast<float>(cs.rcv_bytes - old_conn.rcv_bytes) / 1024 /
          time_delta);
      one_sock_msg->set_retrans_speed(
          static_cast<float>(cs.retrans_bytes - old_conn.retrans_bytes) / 1024 /
          time_delta);
      one_sock_msg->set_retrans_segs_speed(
          static_cast<float>(cs.retrans - old_conn.retrans) / 1024 /
          time_delta);
      one_sock_msg->set_retrans_segs_total_speed(
          static_cast<float>(cs.retrans_total - old_conn.retrans_total) / 1024 /
          time_delta);
    }

    int64_t snd_delta = cs.snd_bytes - old_conn.snd_bytes;
    int64_t retrans_delta = (cs.retrans_bytes - old_conn.retrans_bytes) * 100;
    if (snd_delta > 0) {
      double retrans_rate = static_cast<double>(retrans_delta) / snd_delta;
      one_sock_msg->set_retrans_rate(retrans_rate);
    }
  }
}

void ConnectionMonitor::collect_udp(mw::proto::SystemStats *msg) {
  if (udp_mon_conns_.empty()) {
    return;
  }

  auto netlink = NetLinkTcp::Instance();
  if (0 != netlink->GetAllUdpSockets()) {
    return;
  }

  for (auto local_port : udp_mon_conns_) {
    udp_diag_info udi{};
    if (!netlink->GetOneUdpSocket(local_port, &udi)) {
      continue;
    }

    auto one_sock_msg = msg->add_udp_info();
    one_sock_msg->set_local_port(udi.lport);
    one_sock_msg->set_rcv_buf(udi.rcv_buf);
    one_sock_msg->set_rcv_mem(udi.rcv_mem);
    one_sock_msg->set_drops(udi.drops);
  }
}

int32_t ConnectionMonitor::RunOnce(mw::proto::SystemStats &msg) {
  collect_udp(&msg);

  std::lock_guard<std::mutex> c_guard{lock_};

  if (ros_conns_.empty() && tcp_mon_conns_.empty()) {
    return -1;
  }

  auto netlink = NetLinkTcp::Instance();
  if (0 != netlink->GetAllSockets()) {
    return -1;
  }

  int64_t cur_time_ns = mw_base::SteadyClockNowNs();
  std::map<conn_key, conn_stats> all_conns_cur;
  for (auto &conn : ros_conns_) {
    auto one_sock_msg = msg.add_conn_info();
    one_sock_msg->set_nodename(conn.node);
    one_sock_msg->set_topic(conn.topic);
    one_sock_msg->set_direction(conn.direction);
    one_sock_msg->set_dest_node(conn.dest_uri);
    one_sock_msg->set_proto(conn.transport_type);
    one_sock_msg->set_local_addr(std::string("localhost:") +
                                 std::to_string(conn.local_port));
    one_sock_msg->set_peer_addr(conn.peer_host + std::string(":") +
                                std::to_string(conn.peer_port));

    appendConnInfo(conn.local_port, conn.peer_host, conn.peer_port, cur_time_ns,
                   &all_conns_cur, one_sock_msg);
  }

  for (auto &conn : tcp_mon_conns_) {
    auto one_sock_msg = msg.add_conn_info();
    one_sock_msg->set_nodename("tcp_mon");
    one_sock_msg->set_local_addr(std::string("localhost:") +
                                 std::to_string(conn.local_port));
    one_sock_msg->set_peer_addr(conn.peer_host + std::string(":") +
                                std::to_string(conn.peer_port));
    appendConnInfo(conn.local_port, conn.peer_host, conn.peer_port, cur_time_ns,
                   &all_conns_cur, one_sock_msg);
  }

  all_conns_ = all_conns_cur;
  return 0;
}

}  // namespace system_stats
}  // namespace mw
