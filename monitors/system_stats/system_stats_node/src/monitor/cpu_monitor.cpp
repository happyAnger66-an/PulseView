#include "monitor/cpu_monitor.h"

#include "utils/proc_file.h"
#include "utils/proc_stat.h"
#include "utils/process.h"

namespace mw {
namespace system_stats {

int64_t CpuMonitor::cpu_total_time(const ProcStat::CpuStat &stat) {
  return stat.user + stat.system + stat.idle + stat.nice + stat.io_wait +
         stat.irq + stat.soft_irq + stat.steal;
}

int64_t CpuMonitor::cpu_busy_time(const ProcStat::CpuStat &stat) {
  return stat.user + stat.system + stat.nice + stat.irq + stat.soft_irq +
         stat.steal;
}

int32_t CpuMonitor::collect_one_cpu_stat(const ProcStat::CpuStat &cur,
                                         const ProcStat::CpuStat &prev,
                                         OutputData &output_data) {
  int64_t last_cpu_all = cpu_total_time(prev);
  int64_t last_cpu_busy = cpu_busy_time(prev);

  int64_t now_cpu_all = cpu_total_time(cur);
  int64_t now_cpu_busy = cpu_busy_time(cur);

  if (now_cpu_busy < last_cpu_busy) return -1;

  int64_t busy_delta = now_cpu_busy - last_cpu_busy;
  int64_t all_delta = now_cpu_all - last_cpu_all;

  double cpu_percent =
      static_cast<double>(busy_delta) / static_cast<double>(all_delta) * 100.0;
  OneCpuInfo one_cpu_info{};

  one_cpu_info.cpu_name = cur.cpu_info;
  one_cpu_info.cpu_percent = cpu_percent;

  float user_percent = static_cast<float>(cur.user - prev.user) /
                       static_cast<float>(all_delta) * 100.0;
  one_cpu_info.cpu_user_percent = user_percent;

  float sys_percent = static_cast<float>(cur.system - prev.system) /
                      static_cast<float>(all_delta) * 100.0;
  one_cpu_info.cpu_sys_percent = sys_percent;

  float idle_percent = static_cast<float>(cur.idle - prev.idle) /
                       static_cast<float>(all_delta) * 100.0;
  one_cpu_info.cpu_idle_percent = idle_percent;

  float wait_percent = static_cast<float>(cur.io_wait - prev.io_wait) /
                       static_cast<float>(all_delta) * 100.0;
  one_cpu_info.cpu_wa_percent = wait_percent;

  float ni_percent = static_cast<float>(cur.nice - prev.nice) /
                     static_cast<float>(all_delta) * 100.0;
  one_cpu_info.cpu_ni_percent = ni_percent;

  float si_percent = static_cast<float>(cur.soft_irq - prev.soft_irq) /
                     static_cast<float>(all_delta) * 100.0;
  one_cpu_info.cpu_si_percent = si_percent;

  float hi_percent = static_cast<float>(cur.irq - prev.irq) /
                     static_cast<float>(all_delta) * 100.0;
  one_cpu_info.cpu_hi_percent = hi_percent;

  output_data.cpu_infos.push_back(one_cpu_info);
  return 0;
}

int32_t CpuMonitor::collect_load_info(OutputData &output_data) {
  ProcFile load_avg_file("/proc/loadavg");
  float avg_1, avg_5, avg_15;
  if (!load_avg_file.ReadOneLine(avg_1, avg_5, avg_15)) {
    return -1;
  }

  auto &load = output_data.cpu_load;
  load.load_avg_1 = avg_1;
  load.load_avg_5 = avg_5;
  load.load_avg_15 = avg_15;

  ProcStat::SysInfo sys_info_cur;
  if (sys_info_first_) {
    if (!ProcStat::GetSysInfo(sys_info_prev_)) {
      sys_info_first_ = false;
    }
  } else {
    if (!ProcStat::GetSysInfo(sys_info_cur)) {
      int64_t delta_cs = sys_info_cur.cs - sys_info_prev_.cs;
      float delta_time = (sys_info_cur.ts - sys_info_prev_.ts) / KNsToS;
      load.cs = delta_cs / delta_time;
    }

    load.procs = sys_info_cur.procs;
    load.procs_running = sys_info_cur.procs_running;
    load.procs_blocked = sys_info_cur.procs_blocked;
    sys_info_prev_ = sys_info_cur;
  }
  return 0;
}

int32_t CpuMonitor::collect_all_cpu_stat(OutputData &output_data) {
  if (all_stat_prev_.empty()) {
    ProcStat::AllCpuStat(all_stat_prev_);
    return 0;
  }

  all_stat_cur_.clear();
  ProcStat::AllCpuStat(all_stat_cur_);
  if (all_stat_cur_.size() != all_stat_prev_.size()) {
    return -1;
  }

  int i = 0;  // skip total cpu info
  int cpu_num = all_stat_cur_.size();
  for (i = 0; i < cpu_num; i++) {
    collect_one_cpu_stat(all_stat_cur_[i], all_stat_prev_[i], output_data);
  }

  all_stat_prev_ = all_stat_cur_;
  return 0;
}

int32_t CpuMonitor::RunOnce(OutputData &output_data) {
  ProcStat::CpuStat cpu_stat_now;

  if (first_) {
    if (!ProcStat::Stat(cpu_stat_last_)) {
      return -1;
    }
    first_ = false;
  } else {
    if (!ProcStat::Stat(cpu_stat_now)) {
      return -1;
    }
    int64_t last_cpu_all = cpu_total_time(cpu_stat_last_);
    int64_t last_cpu_busy = cpu_busy_time(cpu_stat_last_);

    int64_t now_cpu_all = cpu_total_time(cpu_stat_now);
    int64_t now_cpu_busy = cpu_busy_time(cpu_stat_now);

    if (now_cpu_busy < last_cpu_busy) return -1;

    int64_t busy_delta = now_cpu_busy - last_cpu_busy;
    int64_t all_delta = now_cpu_all - last_cpu_all;

    double cpu_percent = static_cast<double>(busy_delta) /
                         static_cast<double>(all_delta) * 100.0;

    auto &cpu_data = output_data.cpu;
    cpu_data.cpu_percent = cpu_percent;

    float user_percent =
        static_cast<float>(cpu_stat_now.user - cpu_stat_last_.user) /
        static_cast<float>(all_delta) * 100.0;
    cpu_data.user_percent = user_percent;

    float sys_percent =
        static_cast<float>(cpu_stat_now.system - cpu_stat_last_.system) /
        static_cast<float>(all_delta) * 100.0;
    cpu_data.sys_percent = sys_percent;

    float idle_percent =
        static_cast<float>(cpu_stat_now.idle - cpu_stat_last_.idle) /
        static_cast<float>(all_delta) * 100.0;
    cpu_data.idle_percent = idle_percent;

    float wait_percent =
        static_cast<float>(cpu_stat_now.io_wait - cpu_stat_last_.io_wait) /
        static_cast<float>(all_delta) * 100.0;
    cpu_data.wait_percent = wait_percent;

    float ni_percent =
        static_cast<float>(cpu_stat_now.nice - cpu_stat_last_.nice) /
        static_cast<float>(all_delta) * 100.0;
    cpu_data.ni_percent = ni_percent;

    float si_percent =
        static_cast<float>(cpu_stat_now.soft_irq - cpu_stat_last_.soft_irq) /
        static_cast<float>(all_delta) * 100.0;
    cpu_data.si_percent = si_percent;

    float hi_percent =
        static_cast<float>(cpu_stat_now.irq - cpu_stat_last_.irq) /
        static_cast<float>(all_delta) * 100.0;
    cpu_data.hi_percent = hi_percent;

    cpu_stat_last_ = cpu_stat_now;
  }

  collect_all_cpu_stat(output_data);
  collect_load_info(output_data);
  return 0;
}

}  // namespace system_stats
}  // namespace mw
