#pragma once
#include <unordered_map>
#include "mw_stats/hz.h"
#include "system_stats_protos/system_stats.pb.h"
#include "monitor/monitor_item.h"

namespace mw {
namespace system_stats {
class TopicMonitor : public MonitorItem {
  struct TopicInfo {
    std::string name;
    double cfg_hz = -1;
    double cur_hz = -1;
    double error = -1;
    double min_delta = -1;
    double max_delta = -1;
    double std_dev = -1;
    int32_t window = 0;
  };

 public:
  explicit TopicMonitor(mw::proto::SystemStatsConfig &cfg)
      : MonitorItem(cfg) {}
  int32_t Start() override;
  int32_t RunOnce(mw::proto::SystemStats &msg) override;
  int32_t Stop() override { return 0; }
  const std::string Name() const override { return "TopicMonitor"; }

 private:
  std::shared_ptr<mw::system_stats::HzStats> hz_stats_;
  std::map<std::string, mw::proto::TopicConfig> all_monitor_topics_;
};
}  // namespace system_stats
}  // namespace mw
