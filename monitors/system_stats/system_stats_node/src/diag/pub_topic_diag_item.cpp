#include "pub_topic_diag_item.h"

#include "diagnostic_msgs/msg/key_value.hpp"

namespace mw {
namespace system_stats {

int32_t PubTopicDiagItem::Start(const mw::proto::SystemStatsDiagConfig &cfg) {
  int32_t ret = -1;
  cfg_ = cfg;

  if (cfg.pub_topic_size() > 0) {
    auto &topics_cfg = cfg.pub_topic();

    for (auto &topic_cfg : topics_cfg) {
      if (topic_cfg.has_node()) {
        auto node_name = topic_cfg.node();
        auto topic = topic_cfg.topic();
        auto hz = topic_cfg.hz();
        auto warn = topic_cfg.warn();
        auto error = topic_cfg.error();
        node_topics_cfg_[{node_name, topic}] = {hz, warn, error};
      } else {
        auto topic = topic_cfg.topic();
        auto hz = topic_cfg.hz();
        auto warn = topic_cfg.warn();
        auto error = topic_cfg.error();
        topics_cfg_[topic] = {hz, warn, error};
      }
    }

    ret = 0;
  }

  return ret;
}

int32_t PubTopicDiagItem::Stop() { return 0; }

void PubTopicDiagItem::check_node_pub_topic(
    const OutputDataNodePubInfo &data,
    std::vector<diagnostic_msgs::msg::DiagnosticStatus> &status_vec) {
  auto &node_name = data.node;
  auto &topic = data.topic;
  auto &hz = data.hz;

  if (node_topics_cfg_.find({node_name, topic}) != node_topics_cfg_.end()) {
    auto &cfg = node_topics_cfg_[{node_name, topic}];
    if(hz >= cfg.warn) {
      return;
    }
    diagnostic_msgs::msg::DiagnosticStatus status;
    status.name = "NodePubTopic";
    if (hz < cfg.error) {
      status.level = diagnostic_msgs::msg::DiagnosticStatus::ERROR;
    } else if (hz < cfg.warn) {
      status.level = diagnostic_msgs::msg::DiagnosticStatus::WARN;
    }
    status.values.push_back(
        diagnostic_msgs::build<diagnostic_msgs::msg::KeyValue>()
            .key("node")
            .value(node_name));
    status.values.push_back(
        diagnostic_msgs::build<diagnostic_msgs::msg::KeyValue>()
            .key("topic")
            .value(topic));
    status.values.push_back(
        diagnostic_msgs::build<diagnostic_msgs::msg::KeyValue>()
            .key("value")
            .value(std::to_string(hz)));
    status.values.push_back(
        diagnostic_msgs::build<diagnostic_msgs::msg::KeyValue>()
            .key("expected_value")
            .value(std::to_string(cfg.warn)));
    status.values.push_back(
        diagnostic_msgs::build<diagnostic_msgs::msg::KeyValue>()
            .key("unit")
            .value("Hz"));
    status_vec.push_back(status);
    return;
  }

  if (topics_cfg_.find(topic) != topics_cfg_.end()) {
    auto &cfg = topics_cfg_[topic];
    if(hz >= cfg.warn) {
      return;
    }
    diagnostic_msgs::msg::DiagnosticStatus status;
    status.name = "PubTopic";
    if (hz < cfg.error) {
      status.level = diagnostic_msgs::msg::DiagnosticStatus::ERROR;
    } else if (hz < cfg.warn) {
      status.level = diagnostic_msgs::msg::DiagnosticStatus::WARN;
    }
    status.values.push_back(
        diagnostic_msgs::build<diagnostic_msgs::msg::KeyValue>()
            .key("node")
            .value(node_name));
    status.values.push_back(
        diagnostic_msgs::build<diagnostic_msgs::msg::KeyValue>()
            .key("topic")
            .value(topic));
    status.values.push_back(
        diagnostic_msgs::build<diagnostic_msgs::msg::KeyValue>()
            .key("value")
            .value(std::to_string(hz)));
    status.values.push_back(
        diagnostic_msgs::build<diagnostic_msgs::msg::KeyValue>()
            .key("expected_value")
            .value(std::to_string(cfg.warn)));
    status.values.push_back(
        diagnostic_msgs::build<diagnostic_msgs::msg::KeyValue>()
            .key("unit")
            .value("Hz"));
    status_vec.push_back(status);
    return;
  }
}
int32_t PubTopicDiagItem::Diagnose(
    const OutputData &data,
    std::vector<diagnostic_msgs::msg::DiagnosticStatus> &status_vec) {
  for (const auto &node_pub_info : data.node_pub_infos) {
    check_node_pub_topic(node_pub_info, status_vec);
  }
  return 0;
}

REGISTER_DIAG_ITEM_CLASS(PubTopicDiagItem)

}  // namespace system_stats
}  // namespace mw