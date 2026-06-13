#pragma once

#include <pthread.h>
#include <map>
#include <memory>
#include <string>

#include "system_stats_protos/system_stats_config.pb.h"

namespace mw {
namespace system_stats {

class TopicCfg {
 public:
  TopicCfg();
  static std::shared_ptr<TopicCfg> Instance() {
    pthread_once(&once, &TopicCfg::init);
    return topic_cfg;
  }

  int32_t GetTopicCfg(const std::string &topic, mw::proto::TopicConfig &cfg);
  const std::map<std::string, mw::proto::TopicConfig> &GetAllTopicCfgs() {
    return all_topics;
  }
  std::map<std::string, mw::proto::TopicConfig> all_topics;

 private:
  static void init() { topic_cfg = std::make_shared<TopicCfg>(); }
  static pthread_once_t once;
  static std::shared_ptr<TopicCfg> topic_cfg;
};

}  // namespace system_stats
}  // namespace mw
