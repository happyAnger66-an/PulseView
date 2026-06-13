#pragma once
#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>

#include "diag/diag_item.h"

namespace mw {
namespace system_stats {

class SubTopicDiagItem : public DiagItem {
 public:
  SubTopicDiagItem() : DiagItem() {}
  ~SubTopicDiagItem() = default;

  int32_t Start(const mw::proto::SystemStatsDiagConfig &cfg) override;

  int32_t Stop() override;

  int32_t Diagnose(
      const OutputData &data,
      std::vector<diagnostic_msgs::msg::DiagnosticStatus> &status_vec) override;

 private:
  struct TopicDiagItemConfig {
    float hz = 0.0;
    float warn = 0.0;
    float error = 0.0;
  };

  void check_node_sub_topic(
      const OutputDataNodeSubInfo &data,
      std::vector<diagnostic_msgs::msg::DiagnosticStatus> &status_vec);
  std::unordered_map<NodeTopicKey, TopicDiagItemConfig, StringPairHash,
                     StringPairEqual>
      node_topics_cfg_;
  std::unordered_map<std::string, TopicDiagItemConfig> topics_cfg_;
};

}  // namespace system_stats
}  // namespace mw