#pragma once

#include <string>
#include <unordered_map>
#include "mw_stats_protos/system_stats_protos/system_stats_config.pb.h"

namespace mw {
namespace system_stats {

struct OneTopicHz {
  std::string name;
  double hz_normal;
  double hz_cur;
};

using TopicHzInfo = std::unordered_map<std::string, OneTopicHz>;
bool SystemStatsGetConfig(mw::proto::SystemStatsConfig &config);
bool SystemStatsGetAllTopicsHz(TopicHzInfo &topic_hz_info);

}  // namespace system_stats
}  // namespace mw
