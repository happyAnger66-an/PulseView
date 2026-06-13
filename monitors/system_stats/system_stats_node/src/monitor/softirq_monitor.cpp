#include "monitor/softirq_monitor.h"

#include <glog/logging.h>

#include <iostream>

#include "mw_base/now.h"
#include "utils/utils.h"

namespace mw {
namespace system_stats {

void SoftIrqMonitor::do_irqs(OutputData& msg) {
  ProcFile irqs_proc_file("/proc/interrupts");
  std::vector<std::string> one_irq;
  std::vector<std::vector<std::string>> irqs;
  while (irqs_proc_file.ReadOneLine(one_irq)) {
    irqs.push_back(one_irq);
    one_irq.clear();
  }

  if (irqs.empty()) {
    return;
  }

  auto cpus = irqs[0];
  auto now_ns = mw_base::SteadyClockNowNs();
  for (int i = 1; i < irqs.size(); i++) {
    auto irq_name = irqs[i][0];
    for (int cpu = 0; cpu < cpus.size() - 1; cpu++) {
      if (irqs[i].size() - 1 < cpu + 1) {
        break;
      }

      Irq irq_info{0};
      try {
        irq_info.nums = std::stoll(irqs[i][cpu + 1]);
      } catch (std::invalid_argument const& ex) {
        LOG(WARNING) << "invalid irq num " << irqs[i][cpu + 1];
        break;
      }

      irq_info.ts = now_ns;

      auto cpu_name = cpus[cpu];
      auto key = std::make_pair(cpu_name, irq_name);
      auto old_iter = cpu_irq_infos_.find(key);

      if (old_iter == cpu_irq_infos_.end()) {
        cpu_irq_infos_[key] = irq_info;
        continue;
      }

      auto old_item = old_iter->second;
      double delta_time = (irq_info.ts - old_item.ts) / 1e9;
      int64_t delta_nums = irq_info.nums - old_item.nums;
      cpu_irq_infos_[key] = irq_info;

      if (delta_nums <= 0) {
        continue;
      }

      OutputCpuIrq ci{};
      ci.cpu = cpu_name;
      ci.irq = irq_name;
      ci.speed = delta_nums / delta_time;
      msg.cpu_irq_infos.push_back(ci);
    }
  }

  return;
}

int32_t SoftIrqMonitor::RunOnce(OutputData& msg) {
  ProcFile softirqs_proc_file(std::string("/proc/softirqs"));
  std::vector<std::string> one_softirq;
  std::vector<std::vector<std::string>> softirq;
  while (softirqs_proc_file.ReadOneLine(one_softirq)) {
    softirq.push_back(one_softirq);
    one_softirq.clear();
  }

  for (int i = 0; i < softirq[0].size() - 1; i++) {
    std::string name = softirq[0][i];
    struct SoftIrq info;
    info.cpu_name = name;
    info.hi = std::stoll(softirq[1][i + 1]);
    info.timer = std::stoll(softirq[2][i + 1]);
    info.net_tx = std::stoll(softirq[3][i + 1]);
    info.net_rx = std::stoll(softirq[4][i + 1]);
    info.block = std::stoll(softirq[5][i + 1]);
    info.irq_poll = std::stoll(softirq[6][i + 1]);
    info.tasklet = std::stoll(softirq[7][i + 1]);
    info.sched = std::stoll(softirq[8][i + 1]);
    info.hrtimer = std::stoll(softirq[9][i + 1]);
    info.rcu = std::stoll(softirq[10][i + 1]);
    info.timepoint = boost::chrono::steady_clock::now();

    auto iter = cpu_softirqs_.find(name);
    if (iter != cpu_softirqs_.end()) {
      struct SoftIrq& old = (*iter).second;
      double period = Utils::SteadyTimeToS(info.timepoint, old.timepoint);
      OutputSoftIrq one_softirq_msg{};
      one_softirq_msg.cpu = info.cpu_name;
      one_softirq_msg.hi = (info.hi - old.hi) / period;  // increment
      one_softirq_msg.timer = (info.timer - old.timer) / period;
      one_softirq_msg.net_tx = (info.net_tx - old.net_tx) / period;
      one_softirq_msg.net_rx = (info.net_rx - old.net_rx) / period;
      one_softirq_msg.block = (info.block - old.block) / period;
      one_softirq_msg.irq_poll = (info.irq_poll - old.irq_poll) / period;
      one_softirq_msg.tasklet = (info.tasklet - old.tasklet) / period;
      one_softirq_msg.sched = (info.sched - old.sched) / period;
      one_softirq_msg.hrtimer = (info.hrtimer - old.hrtimer) / period;
      one_softirq_msg.rcu = (info.rcu - old.rcu) / period;
      msg.softirq_infos.push_back(one_softirq_msg);
    }
    cpu_softirqs_[name] = info;
  }

  do_irqs(msg);
  return 0;
}

}  // namespace system_stats
}  // namespace mw
