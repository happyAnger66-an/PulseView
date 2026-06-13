#pragma once

#include "monitor/monitor_item.h"

namespace mw {
namespace system_stats {

class MemMonitor : public MonitorItem {
 public:
  explicit MemMonitor(mw::proto::SystemStatsConfig &cfg)
      : MonitorItem(cfg) {}
  int32_t Start() override { return 0; }
  int32_t RunOnce(OutputData &output_data) override;
  int32_t Stop() override { return 0; }
  const std::string Name() const override { return "MemMonitor"; }
};
}  // namespace system_stats
}  // namespace mw
