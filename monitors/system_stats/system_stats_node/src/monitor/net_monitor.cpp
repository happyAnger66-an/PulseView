#include "monitor/net_monitor.h"

#include <glog/logging.h>

#include <boost/range/combine.hpp>

#include "mw_base/now.h"
#include "utils/utils.h"

namespace mw {
namespace system_stats {

constexpr int kNetdevWords = 13;
constexpr int kTcpSnmpNums = 5;
constexpr int kUdpSnmpNums = 5;
constexpr int kIpSnmpNums = 13;

void NetMonitor::do_snmp_ip(OutputData &msg,
                            const std::vector<std::string> &names,
                            const std::vector<std::string> &values) {
  if (names.size() != values.size()) {
    LOG(ERROR) << "snmp ip names: " << names.size()
               << "!= values: " << values.size();
    return;
  }

  struct ip_snmp cur_ip_snmp {};
  int items = 0;
  cur_ip_snmp.timepoint = mw_base::SteadyClockNowNs();
  for (auto item : boost::combine(names, values)) {
    std::string key;
    std::string value;
    boost::tie(key, value) = item;

    if (key == "InHdrErrors") {
      cur_ip_snmp.in_hdr_errs = std::stoll(value);
      items++;
    } else if (key == "InAddrErrors") {
      cur_ip_snmp.in_addr_errs = std::stoll(value);
      items++;
    } else if (key == "InUnknownProtos") {
      cur_ip_snmp.in_unkown_protos = std::stoll(value);
      items++;
    } else if (key == "InDiscards") {
      cur_ip_snmp.in_discards = std::stoll(value);
      items++;
    } else if (key == "OutDiscards") {
      cur_ip_snmp.out_discards = std::stoll(value);
      items++;
    } else if (key == "OutNoRoutes") {
      cur_ip_snmp.out_no_routes = std::stoll(value);
      items++;
    } else if (key == "ReasmTimeout") {
      cur_ip_snmp.reasm_timeout = std::stoll(value);
      items++;
    } else if (key == "ReasmReqds") {
      cur_ip_snmp.reasm_reqds = std::stoll(value);
      items++;
    } else if (key == "ReasmOKs") {
      cur_ip_snmp.reasm_oks = std::stoll(value);
      items++;
    } else if (key == "ReasmFails") {
      cur_ip_snmp.reasm_fails = std::stoll(value);
      items++;
    } else if (key == "FragOKs") {
      cur_ip_snmp.frag_oks = std::stoll(value);
      items++;
    } else if (key == "FragFails") {
      cur_ip_snmp.frag_fails = std::stoll(value);
      items++;
    } else if (key == "FragCreates") {
      cur_ip_snmp.frag_creates = std::stoll(value);
      items++;
    }
  }

  if (items != kIpSnmpNums) {
    return;
  }

  if (!ip_first_) {
    auto &snmp_ip = msg.ip_info;
    float time_delta = (cur_ip_snmp.timepoint - old_ip_snmp_.timepoint) / 1e9;
    if (time_delta > 0) {
      snmp_ip.in_hdr_errs_speed =
          (cur_ip_snmp.in_hdr_errs - old_ip_snmp_.in_hdr_errs) / time_delta;
      snmp_ip.in_addr_errs_speed =
          (cur_ip_snmp.in_addr_errs - old_ip_snmp_.in_addr_errs) / time_delta;
      snmp_ip.in_unkown_protos_speed =
          (cur_ip_snmp.in_unkown_protos - old_ip_snmp_.in_unkown_protos) /
          time_delta;
      snmp_ip.in_discards_speed =
          (cur_ip_snmp.in_discards - old_ip_snmp_.in_discards) / time_delta;
      snmp_ip.out_discards_speed =
          (cur_ip_snmp.out_discards - old_ip_snmp_.out_discards) / time_delta;
      snmp_ip.out_no_routes_speed =
          (cur_ip_snmp.out_no_routes - old_ip_snmp_.out_no_routes) / time_delta;
      snmp_ip.reasm_timeout_speed =
          (cur_ip_snmp.reasm_timeout - old_ip_snmp_.reasm_timeout) / time_delta;
      snmp_ip.reasm_reqds_speed =
          ((cur_ip_snmp.reasm_reqds - old_ip_snmp_.reasm_reqds) / time_delta);
      snmp_ip.reasm_oks_speed =
          (cur_ip_snmp.reasm_oks - old_ip_snmp_.reasm_oks) / time_delta;
      snmp_ip.reasm_fails_speed =
          (cur_ip_snmp.reasm_fails - old_ip_snmp_.reasm_fails) / time_delta;
      snmp_ip.frag_oks_speed =
          (cur_ip_snmp.frag_oks - old_ip_snmp_.frag_oks) / time_delta;
      snmp_ip.frag_fails_speed =
          (cur_ip_snmp.frag_fails - old_ip_snmp_.frag_fails) / time_delta;
      snmp_ip.frag_creates_speed =
          (cur_ip_snmp.frag_creates - old_ip_snmp_.frag_creates) / time_delta;
    }
  }

  old_ip_snmp_ = cur_ip_snmp;
  ip_first_ = false;
  return;
}

void NetMonitor::do_snmp_tcp(OutputData &msg,
                             const std::vector<std::string> &names,
                             const std::vector<std::string> &values) {
  if (names.size() != values.size()) {
    LOG(ERROR) << "snmp tcp names: " << names.size()
               << "!= values: " << values.size();
    return;
  }

  struct tcp_snmp cur_tcp_snmp {};
  int items = 0;
  cur_tcp_snmp.timepoint = mw_base::SteadyClockNowNs();
  for (auto item : boost::combine(names, values)) {
    std::string key;
    std::string value;
    boost::tie(key, value) = item;

    if (key == "InSegs") {
      cur_tcp_snmp.in = std::stoll(value);
      items++;
    } else if (key == "OutSegs") {
      cur_tcp_snmp.out = std::stoll(value);
      items++;
    } else if (key == "RetransSegs") {
      cur_tcp_snmp.retrans = std::stoll(value);
      items++;
    } else if (key == "InErrs") {
      cur_tcp_snmp.in_errs = std::stoll(value);
      items++;
    } else if (key == "OutRsts") {
      cur_tcp_snmp.out_rsts = std::stoll(value);
      items++;
    }
  }

  if (items != kTcpSnmpNums) {
    return;
  }

  if (!tcp_first_) {
    auto &snmp_tcp = msg.tcp_info;
    float time_delta = (cur_tcp_snmp.timepoint - old_tcp_snmp_.timepoint) / 1e9;
    snmp_tcp.in_speed = (cur_tcp_snmp.in - old_tcp_snmp_.in) / time_delta;
    snmp_tcp.out_speed = (cur_tcp_snmp.out - old_tcp_snmp_.out) / time_delta;
    snmp_tcp.retrans_speed =
        (cur_tcp_snmp.retrans - old_tcp_snmp_.retrans) / time_delta;
    snmp_tcp.in_errs = cur_tcp_snmp.in_errs;
    snmp_tcp.out_rsts = cur_tcp_snmp.out_rsts;
  }

  old_tcp_snmp_ = cur_tcp_snmp;
  tcp_first_ = false;
  return;
}

void NetMonitor::do_snmp_udp(OutputData &msg,
                             const std::vector<std::string> &names,
                             const std::vector<std::string> &values) {
  if (names.size() != values.size()) {
    LOG(ERROR) << "snmp udp names: " << names.size()
               << "!= values: " << values.size();
    return;
  }

  struct udp_snmp cur_udp_snmp {};
  int items = 0;
  cur_udp_snmp.timepoint = mw_base::SteadyClockNowNs();
  for (auto item : boost::combine(names, values)) {
    std::string key;
    std::string value;
    boost::tie(key, value) = item;

    if (key == "InDatagrams") {
      cur_udp_snmp.in = std::stoll(value);
      items++;
    } else if (key == "OutDatagrams") {
      cur_udp_snmp.out = std::stoll(value);
      items++;
    } else if (key == "InErrors") {
      cur_udp_snmp.in_errs = std::stoll(value);
      items++;
    } else if (key == "RcvbufErrors") {
      cur_udp_snmp.rcvbuf_errs = std::stoll(value);
      items++;
    } else if (key == "SndbufErrors") {
      cur_udp_snmp.sndbuf_errs = std::stoll(value);
      items++;
    }
  }

  if (items != kUdpSnmpNums) {
    return;
  }

  if (!udp_first_) {
    auto &snmp_udp = msg.udp_info;
    float time_delta = (cur_udp_snmp.timepoint - old_udp_snmp_.timepoint) / 1e9;
    snmp_udp.in_speed = (cur_udp_snmp.in - old_udp_snmp_.in) / time_delta;
    snmp_udp.out_speed = (cur_udp_snmp.out - old_udp_snmp_.out) / time_delta;
    snmp_udp.in_errs =
        (cur_udp_snmp.in_errs - old_udp_snmp_.in_errs) / time_delta;
    snmp_udp.rcvbuf_errs = cur_udp_snmp.rcvbuf_errs;
    snmp_udp.sndbuf_errs = cur_udp_snmp.sndbuf_errs;
  }

  old_udp_snmp_ = cur_udp_snmp;
  udp_first_ = false;
  return;
}

void NetMonitor::do_snmp(OutputData &msg) {
  ProcFile snmp_proc_file(std::string("/proc/net/snmp"));
  std::vector<std::string> names{};
  std::vector<std::string> values{};
  std::vector<std::string> all_lines{};
  int status = 0;

  while (snmp_proc_file.ReadOneLine(all_lines)) {
    std::string name = all_lines[0];
    if (name.find(':') == name.size() - 1 && name == "Tcp:") {
      if (status == 0) {
        names = all_lines;
        status = 1;
      } else {
        values = all_lines;
        status = 0;
        do_snmp_tcp(msg, names, values);
      }
    } else if (name.find(':') == name.size() - 1 && name == "Udp:") {
      if (status == 0) {
        names = all_lines;
        status = 1;
      } else {
        values = all_lines;
        status = 0;
        do_snmp_udp(msg, names, values);
      }
    } else if (name.find(':') == name.size() - 1 && name == "Ip:") {
      if (status == 0) {
        names = all_lines;
        status = 1;
      } else {
        values = all_lines;
        status = 0;
        do_snmp_ip(msg, names, values);
      }
    }

    all_lines.clear();
  }
}

void NetMonitor::do_tcp_ext(OutputData &msg,
                            const std::vector<std::string> &names,
                            const std::vector<std::string> &values) {
  /*if (names.size() != values.size()) {
    LOG(ERROR) << "tcp ext names: " << names.size()
               << "!= values: " << values.size();
    return;
  }

  int64_t cur_timestamp_ns = mw_base::SteadyClockNowNs();
  std::map<std::string, int64_t> items;
  int i = 0;
  for (auto item : boost::combine(names, values)) {
    i++;
    if (i == 1) {  // skip TcpExt:
      continue;
    }
    std::string key;
    std::string value;
    boost::tie(key, value) = item;
    int64_t i_value = std::stoll(value);
    items[key] = i_value;
  }

  if (last_tcp_ext_time_ != -1) {
    float time_delta = (cur_timestamp_ns - last_tcp_ext_time_) / 1e9;
    auto tcp_ext_msg = msg->mutable_tcp_ext();
    for (auto iter : items) {
      auto item_msg = tcp_ext_msg->add_item();
      auto name = iter.first;
      item_msg->set_name(name);
      auto old_value = tcp_exts_[name];
      item_msg->set_value((iter.second - old_value) / time_delta);
    }
  }

  tcp_exts_ = items;
  last_tcp_ext_time_ = cur_timestamp_ns;
  return;*/
}

void NetMonitor::do_ip_ext(OutputData &msg,
                           const std::vector<std::string> &names,
                           const std::vector<std::string> &values) {
  /*if (names.size() != values.size()) {
    LOG(ERROR) << "ip ext names: " << names.size()
               << "!= values: " << values.size();
    return;
  }

  int64_t cur_timestamp_ns = mw_base::SteadyClockNowNs();
  std::map<std::string, int64_t> items;
  int i = 0;
  for (auto item : boost::combine(names, values)) {
    i++;
    if (i == 1) {  // Skip Ipext:
      continue;
    }
    std::string key;
    std::string value;
    boost::tie(key, value) = item;
    int64_t i_value = std::stoll(value);
    items[key] = i_value;
  }

  if (last_ip_ext_time_ != -1) {
    float time_delta = (cur_timestamp_ns - last_ip_ext_time_) / 1e9;
    auto tcp_ext_msg = msg->mutable_ip_ext();
    for (auto iter : items) {
      auto item_msg = tcp_ext_msg->add_item();
      auto name = iter.first;
      item_msg->set_name(name);
      auto old_value = ip_exts_[name];
      item_msg->set_value((iter.second - old_value) / time_delta);
    }
  }

  ip_exts_ = items;
  last_ip_ext_time_ = cur_timestamp_ns;
  return;*/
}

void NetMonitor::do_netstat(OutputData &msg) {
  ProcFile netstat_proc_file(std::string("/proc/net/netstat"));
  std::vector<std::string> names{};
  std::vector<std::string> values{};
  std::vector<std::string> all_lines{};
  int status = 0;

  while (netstat_proc_file.ReadOneLine(all_lines)) {
    std::string name = all_lines[0];
    if (name == "TcpExt:") {
      if (status == 0) {
        names = all_lines;
        status = 1;
      } else {
        values = all_lines;
        status = 0;
        do_tcp_ext(msg, names, values);
      }
    } else if (name == "IpExt:") {
      if (status == 0) {
        names = all_lines;
        status = 1;
      } else {
        values = all_lines;
        status = 0;
        do_ip_ext(msg, names, values);
      }
    }

    all_lines.clear();
  }
}

int32_t NetMonitor::RunOnce(OutputData &msg) {
  ProcFile netdev_proc_file(std::string("/proc/net/dev"));
  std::vector<std::string> all_lines;
  while (netdev_proc_file.ReadOneLine(all_lines)) {
    std::string name = all_lines[0];
    if (name.find(':') == name.size() - 1 && all_lines.size() >= kNetdevWords) {
      struct netdev_info info;
      name.pop_back();
      info.name = name;
      info.rcv_bytes = std::stoll(all_lines[1]);
      info.rcv_pkts = std::stoll(all_lines[2]);
      info.err_in = std::stoll(all_lines[3]);
      info.drop_in = std::stoll(all_lines[4]);
      info.snd_bytes = std::stoll(all_lines[9]);
      info.snd_pkts = std::stoll(all_lines[10]);
      info.err_out = std::stoll(all_lines[11]);
      info.drop_out = std::stoll(all_lines[12]);
      info.timepoint = boost::chrono::steady_clock::now();

      auto iter = net_devices_.find(name);
      if (iter != net_devices_.end()) {
        struct netdev_info &old = (*iter).second;
        double period = Utils::SteadyTimeToS(info.timepoint, old.timepoint);
        OutputNetInfo one_net_msg{};
        one_net_msg.name = info.name;
        one_net_msg.send_rate =
            (info.snd_bytes - old.snd_bytes) / 1024.0 / period;  // KB/s
        one_net_msg.rcv_rate =
            (info.rcv_bytes - old.rcv_bytes) / 1024.0 / period;  // KB/s
        one_net_msg.send_pkts_rate = (info.snd_pkts - old.snd_pkts) / period;
        one_net_msg.rcv_pkts_rate = (info.rcv_pkts - old.rcv_pkts) / period;
        msg.net_infos.push_back(one_net_msg);
      }
      net_devices_[name] = info;
    }
    all_lines.clear();
  }

  do_snmp(msg);
  //do_netstat(msg);
  return 0;
}

}  // namespace system_stats
}  // namespace mw
