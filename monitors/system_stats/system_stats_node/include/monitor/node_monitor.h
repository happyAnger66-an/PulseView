#pragma once

#include <unordered_map>

#include "monitor/monitor_item.h"
#include "utils/process.h"

namespace mw {
namespace system_stats {
class NodeMonitor : public MonitorItem {
 public:
  explicit NodeMonitor(mw::proto::SystemStatsConfig &cfg) : MonitorItem(cfg) {}
  int32_t Start() override { return 0; }
  int32_t RunOnce(OutputData &msg) override;
  int32_t Stop() override { return 0; }
  const std::string Name() const override { return "NodeMonitor"; }

 private:
  bool is_digits(const std::string &str);
  OutputNodeInfo *AddCpuTime(const std::string &node_name, pid_t pid,
                             OutputData &msg);
  OutputNodeInfo *AddMemUsed(const std::string &node_name, pid_t pid,
                             OutputData &msg, OutputNodeInfo *one_node_msg);
  bool AddIoStat(const std::string &node_name, pid_t pid,
                 OutputNodeInfo *one_node_msg);
  bool AddSchedTimes(const std::string &node_name, pid_t pid,
                     OutputNodeInfo *one_node_msg);
  bool AddProcStatus(const std::string &node_name, pid_t pid,
                     OutputNodeInfo *one_node_msg);
  bool AddNodeSockets(const std::string &node_name, pid_t pid,
                      OutputNodeInfo *one_node_msg);
  std::map<int, Process::CpuTime> nodes_cpu_;
  std::map<int, Process::SchedTime> nodes_sched_time_;
  std::map<int, Process::IoCounter> nodes_io_;
};
}  // namespace system_stats
}  // namespace mw