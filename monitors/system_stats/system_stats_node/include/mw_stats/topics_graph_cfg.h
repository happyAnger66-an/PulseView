#pragma once

#include <memory>
#include <string>
#include <unordered_map>

#include "mw_stats_protos/topics_graph.pb.h"
#include "mw_comm/onboard_config.h"
#include "mw_comm/singleton.h"

namespace mw {
namespace system_stats {

class TopicsGraphCfg : public mw_comm::Singleton<TopicsGraphCfg> {
  friend class mw_comm::Singleton<TopicsGraphCfg>;

 public:
  std::string FindInputTopic(const std::string &topic) {
    const auto &iter = topic_graph_.find(topic);
    if (iter != topic_graph_.end()) {
      return iter->second;
    }

    return "";
  }

 private:
  TopicsGraphCfg() {
    topic_graph_pb_ =
        mw_comm::CreateConfig<mw::common::TopicGraph>("topics_graph");
    for (auto &edge : topic_graph_pb_.edge()) {
      topic_graph_[edge.topic()] = edge.input_topic();
    }
  }
  mw::common::TopicGraph topic_graph_pb_;
  std::unordered_map<std::string, std::string> topic_graph_;
};

}  // namespace system_stats
}  // namespace mw
