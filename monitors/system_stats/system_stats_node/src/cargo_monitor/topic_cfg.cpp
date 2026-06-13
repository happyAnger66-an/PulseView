#include <pthread.h>
#include <map>
#include <memory>
#include <string>

#include "mw_monitor_protos/system_stats_protos/system_stats_config.pb.h"
#include "mw_base/base_dir.h"
#include "absl/strings/str_format.h"
#include "mw_comm/onboard_config.h"
#include "mw_monitor/topic_cfg.h"
#include "mw_proto_util/proto_io.h"

namespace mw {
namespace system_stats {

TopicCfg::TopicCfg() {
  mw::proto::SystemStatsConfig system_stats_pb_config;
  system_stats_pb_config =
      mw_comm::CreateConfig<mw::proto::SystemStatsConfig>(
          "system_stats_config");

  for (auto &topic : system_stats_pb_config.topic()) {
    all_topics[topic.name()] = topic;
  }
}

int32_t TopicCfg::GetTopicCfg(const std::string &topic,
                              mw::proto::TopicConfig &cfg) {
  if (all_topics.find(topic) == all_topics.end()) {
    return -1;
  }

  cfg = all_topics[topic];
  return 0;
}

pthread_once_t TopicCfg::once = PTHREAD_ONCE_INIT;
std::shared_ptr<TopicCfg> TopicCfg::topic_cfg = nullptr;

}  // namespace system_stats
}  // namespace mw
