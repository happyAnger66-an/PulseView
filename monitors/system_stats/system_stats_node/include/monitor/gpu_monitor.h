#pragma once

#include <nvml.h>
#include "monitor/monitor_item.h"

namespace mw {
namespace system_stats {
class GpuMonitor : public MonitorItem {
  struct GpuInfo {
    uint32_t index;
    std::string device_name;
    std::string bus_id;
    double temperature;
    double temperature_shutdown;
    double temperature_slowdown;
    double power_usage;
    double power_capability;
    double memory_total;
    double memory_free;
    double memory_used;
  };

 public:
  explicit GpuMonitor(mw::proto::SystemStatsConfig &cfg)
      : MonitorItem(cfg) {}

  template <typename Func>
  Func Load(void *handle, const std::string &name) const;
  int32_t Start() override;
  int32_t RunOnce(OutputData &output_data) override;
  int32_t Stop() override { return 0; }
  const std::string Name() const override { return "GpuMonitor"; }

 private:
  bool GpuProcesses(nvmlDevice_t device, OutputData &output_data);
};
}  // namespace system_stats
}  // namespace mw
