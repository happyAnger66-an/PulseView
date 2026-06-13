#pragma once
#include <unordered_map>
#include "monitor/monitor_item.h"
#include "utils/thread.h"

#include "mw_shm/shm_stats/shm_thread_mgr.h"

namespace mw {
namespace system_stats {

class ThreadMonitor : public MonitorItem {
 public:
  explicit ThreadMonitor(mw::proto::SystemStatsConfig &cfg)
      : MonitorItem(cfg) {}
  int32_t Start() override;
  int32_t RunOnce(mw::proto::SystemStats &msg) override;
  int32_t Stop() override;
  const std::string Name() const override { return "ThreadMonitor"; }

 private:
  std::shared_ptr<shm_stats::ShmThreadMgr> shm_thread_mgr_ = nullptr;
  bool is_digits(const std::string &str);
  int32_t AddOneThread(int pid, int tid, int type,
                       mw::proto::SystemStats &msg);
  int32_t AddOneProcessThreads(int pid,
                               const std::vector<std::pair<int, int>> &threads,
                               mw::proto::SystemStats &msg);
  int32_t AddAllThreads(
      const std::vector<int> &pids,
      const std::vector<std::vector<std::pair<int, int>>> &all_threads,
      mw::proto::SystemStats &msg);
  mw::proto::NodeThreadInfo *AddCpuTime(pid_t pid, pid_t tid, int type,
                                           mw::proto::SystemStats &msg);
  bool AddIoStat(pid_t pid, pid_t tid, mw::proto::NodeThreadInfo &msg);
  bool AddSchedTimes(pid_t pid, pid_t tid, mw::proto::NodeThreadInfo &msg);
  bool AddStatus(pid_t pid, pid_t tid, mw::proto::NodeThreadInfo &msg);
  std::map<int, Thread::CpuTime> threads_cpu_;
  std::map<int, Thread::SchedTime> threads_sched_time_;
  std::map<int, Thread::IoCounter> threads_io_;
};

}  // namespace system_stats
}  // namespace mw
