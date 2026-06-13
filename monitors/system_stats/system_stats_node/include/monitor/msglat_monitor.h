#pragma once

#include <unordered_map>
#include "mw_stats/msg_lat_cfg.h"
#include "mw_stats_protos/system_info.pb.h"
#include "monitor/monitor_item.h"
#include "mw_shm/shm_stats/shm_latency_mgr.h"

namespace mw {
namespace system_stats {

class MsgLatMonitor : public MonitorItem {
 public:
  explicit MsgLatMonitor(mw::proto::SystemStatsConfig &cfg)
      : MonitorItem(cfg) {}
  int32_t Start() override;
  int32_t RunOnce(mw::proto::SystemStats &msg) override;
  int32_t Stop() override { return 0; }
  const std::string Name() const override { return "MsgLatMonitor"; }

 private:
  int global_latency_ = 100;
  std::shared_ptr<shm_stats::ShmSubLatMgr> shm_sub_lat_mgr_;
  std::shared_ptr<MsgLatCfg> msg_lat_cfg_;
  std::map<std::string, mw::proto::NodeLatencyConfig> all_msg_cfg_;
  void check_one_topic_msg(const std::string &node, const std::string &topic,
                           mw::proto::SystemStats &msg);
};

}  // namespace system_stats
}  // namespace mw
