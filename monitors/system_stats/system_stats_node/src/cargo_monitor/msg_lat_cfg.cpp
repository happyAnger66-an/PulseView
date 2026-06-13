#include <pthread.h>
#include <map>
#include <memory>
#include <string>

#include "mw_monitor_protos/system_stats_protos/system_stats_config.pb.h"
#include "mw_base/base_dir.h"
#include "absl/strings/str_format.h"
#include "mw_comm/onboard_config.h"
#include "mw_monitor/msg_lat_cfg.h"
#include "mw_proto_util/proto_io.h"

namespace mw {
namespace system_stats {

MsgLatCfg::MsgLatCfg() {
  mw::proto::SystemStatsConfig system_stats_pb_config;
  system_stats_pb_config =
      mw_comm::CreateConfig<mw::proto::SystemStatsConfig>(
          "system_stats_config");

  for (auto &one_msg_lat : system_stats_pb_config.node_msg_lat()) {
    all_msg_lats_[get_key(one_msg_lat.node(), one_msg_lat.topic())] =
        one_msg_lat;
  }

  global_msg_lat_ = system_stats_pb_config.msg_lat_cfg().latency();
  global_switch_ = system_stats_pb_config.msg_lat_cfg().global_switch();
}

bool MsgLatCfg::IsMsgLatCfg(const std::string &node_name,
                            const std::string &topic) {
  if (all_msg_lats_.find(get_key(node_name, topic)) == all_msg_lats_.end()) {
    return false;
  }

  return true;
}

int32_t MsgLatCfg::GetMsgLatCfg(const std::string &node_name,
                                const std::string &topic,
                                mw::proto::NodeLatencyConfig &cfg) {
  std::string key = get_key(node_name, topic);
  if (all_msg_lats_.find(key) == all_msg_lats_.end()) {
    return -1;
  }

  cfg = all_msg_lats_[key];
  return 0;
}

pthread_once_t MsgLatCfg::once = PTHREAD_ONCE_INIT;
std::shared_ptr<MsgLatCfg> MsgLatCfg::lat_cfg = nullptr;

}  // namespace system_stats
}  // namespace mw
