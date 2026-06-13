#pragma once
#include <boost/chrono.hpp>

#include "monitor/monitor_item.h"
#include "utils/proc_stat.h"
#include "outputs/output_datas.h"

namespace mw {
namespace system_stats {

class CpuMonitor : public MonitorItem {
 public:
  explicit CpuMonitor(mw::proto::SystemStatsConfig &cfg)
      : MonitorItem(cfg) {}
  int32_t Start() override { return 0; }
  int32_t RunOnce(OutputData &output_data) override;
  int32_t Stop() override { return 0; }
  const std::string Name() const override { return "CpuMonitor"; }
  ~CpuMonitor() override = default;

  private:
  int64_t cpu_busy_time(const ProcStat::CpuStat &stat);
  int64_t cpu_total_time(const ProcStat::CpuStat &stat);
  int32_t collect_all_cpu_stat(OutputData &output_data);
  int32_t collect_one_cpu_stat(const ProcStat::CpuStat &cur,
                               const ProcStat::CpuStat &prev,
                               OutputData &output_data);
  int32_t collect_load_info(OutputData &output_data);
  bool first_ = true;
  bool sys_info_first_ = true;
  ProcStat::CpuStat cpu_stat_last_;
  ProcStat::SysInfo sys_info_prev_;
  std::vector<ProcStat::CpuStat> all_stat_prev_;
  std::vector<ProcStat::CpuStat> all_stat_cur_;
};

}  // namespace system_stats
}  // namespace mw