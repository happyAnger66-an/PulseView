#pragma once

#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <tuple>
#include <vector>

#include "monitor/monitor_item.h"
#include "utils/ros_utils.h"

namespace mw {
namespace system_stats {
class ConnectionMonitor : public MonitorItem {
 public:
  explicit ConnectionMonitor(const mw::proto::SystemStatsConfig &cfg)
      : MonitorItem(cfg) {
    for (auto tcp_mon_conn : cfg.tcp_mon_cfg()) {
      tcp_conn_cfg tc{0};
      tc.local_port = tcp_mon_conn.local_port();
      tc.peer_host = tcp_mon_conn.peer_host();
      tc.peer_port = tcp_mon_conn.peer_port();
      tcp_mon_conns_.push_back(tc);
    }

    for (auto udp_mon_conn : cfg.udp_mon_cfg()) {
      udp_mon_conns_.push_back(udp_mon_conn.local_port());
    }
  }
  int32_t Start() override;
  int32_t RunOnce(mw::proto::SystemStats &msg) override;
  int32_t Stop() override;
  const std::string Name() const override { return "ConnectionMonitor"; }

 private:
  struct ros_conn_info {
    std::string node;
    std::string topic;
    std::string direction;
    std::string dest_uri;
    std::string transport_type;
    int local_port;
    std::string peer_host;
    int peer_port;
  };

  struct conn_stats {
    int local_port;
    std::string peer_host;
    int peer_port;
    int64_t snd_bytes;
    int64_t rcv_bytes;
    int64_t retrans_total;
    int64_t retrans;
    int64_t retrans_bytes;
    int64_t update_ns;
  };

  struct tcp_conn_cfg {
    int local_port;
    std::string peer_host;
    int peer_port;
  };
  bool stop_ = false;
  std::mutex lock_;
  std::vector<ros_conn_info> ros_conns_;
  std::vector<tcp_conn_cfg> tcp_mon_conns_;
  std::vector<int> udp_mon_conns_;
  std::unique_ptr<std::thread> thread_;
  using conn_key = std::tuple<int, std::string, int>;
  std::map<conn_key, conn_stats> all_conns_;
  bool filter_connection(const RosUtils::RosConnection &conn);
  void collect_connections();
  void collect_udp(mw::proto::SystemStats *msg);
  void appendConnInfo(int local_port, const std::string &peer_host,
                      int peer_port, int64_t cur_time_ns,
                      std::map<conn_key, conn_stats> *all_conns_cur,
                      mw::proto::ConnInfo *one_sock_msg);
};

}  // namespace system_stats
}  // namespace mw
