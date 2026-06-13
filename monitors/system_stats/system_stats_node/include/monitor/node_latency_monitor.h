#pragma once

#include <memory>
#include <string>

#include "monitor/monitor_item.h"
#include "mw_shm/shm_stats/shm_cnts_mgr.h"

namespace mw {
namespace system_stats {

class NodeLatencyMonitor : public MonitorItem {
 public:
  explicit NodeLatencyMonitor(mw::proto::SystemStatsConfig &cfg)
      : MonitorItem(cfg) {}
  int32_t Start() override;
  int32_t RunOnce(OutputData &msg) override;
  int32_t Stop() override;
  const std::string Name() const override { return "NodeLatencyMonitor"; }

 private:
  std::shared_ptr<shm_stats::ShmCntsMgr> shm_cnts_mgr_ = nullptr;
  int32_t add_pub_info(const std::string &node, const std::string &topic,
                       const shm_stats::ShmCntRecord &record, OutputData &msg);
  int32_t add_sub_info(const std::string &node, const std::string &topic,
                       const shm_stats::ShmCntRecord &record, OutputData &msg);
};

}  // namespace system_stats
}  // namespace mw
