#pragma once
#include <boost/chrono.hpp>

#include "monitor/monitor_item.h"
#include "utils/libsensors_chip.h"
#include "utils/proc_stat.h"

namespace mw {
namespace system_stats {

class CpuTempMonitor : public MonitorItem {
 public:
  explicit CpuTempMonitor(mw::proto::SystemStatsConfig &cfg)
      : MonitorItem(cfg) {}
  int32_t Start() override;
  int32_t RunOnce(OutputData &msg) override;
  int32_t Stop() override;
  const std::string Name() const override { return "CpuTempMonitor"; }

 private:
  // All of the enumerated sensor chips
  std::vector<SensorChipPtr> sensor_chips_;
};

}  // namespace system_stats
}  // namespace mw