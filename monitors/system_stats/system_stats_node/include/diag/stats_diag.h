#pragma once
#include <cstdint>
#include <iostream>
#include <vector>

#include "diag_item.h"
#include "diagnostic_msgs/msg/diagnostic_array.hpp"
#include "diagnostic_msgs/msg/key_value.hpp"
#include "outputs/output_datas.h"
#include "rclcpp/rclcpp.hpp"
#include "system_stats_protos/system_stats_diag_config.pb.h"

namespace mw {
namespace system_stats {

class StatsDiag {
 public:
  StatsDiag(const mw::proto::SystemStatsDiagConfig &cfg);

  ~StatsDiag() = default;

  int32_t Run(rclcpp::Node *node, const OutputData &data);

 private:
  std::shared_ptr<rclcpp::Publisher<diagnostic_msgs::msg::DiagnosticArray>>
      diag_pub_;
  std::vector<diagnostic_msgs::msg::DiagnosticStatus> status_vec_;

  std::unordered_map<std::string, std::unique_ptr<DiagItem>> diag_items_;
};

}  // namespace system_stats
}  // namespace mw