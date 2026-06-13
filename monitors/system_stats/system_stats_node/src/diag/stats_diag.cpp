#include "diag/stats_diag.h"

#include <glog/logging.h>

#include <cstdint>
#include <iostream>
#include <vector>

#include "mw_base/now.h"

namespace mw {
namespace system_stats {

StatsDiag::StatsDiag(const mw::proto::SystemStatsDiagConfig &cfg) {
  DiagItemFactory::getInstance().CreateDiagItems(diag_items_);

  int ret = 0;
  for (auto diag_iter = diag_items_.begin(); diag_iter != diag_items_.end();
       ret = 0) {
    ret = diag_iter->second->Start(cfg);
    if (ret != 0) {
      LOG(ERROR) << "diag item " << diag_iter->first << " start failed !!!";
      diag_iter = diag_items_.erase(diag_iter);
    } else {
      diag_iter++;
    }
  }
}

int32_t StatsDiag::Run(rclcpp::Node *node, const OutputData &data) {
  status_vec_.clear();
  for (const auto &[name, diag_item] : diag_items_) {
    diag_item->Diagnose(data, status_vec_);
  }

  diagnostic_msgs::msg::DiagnosticArray diag_array;
  diag_array.header.stamp = rclcpp::Clock(RCL_ROS_TIME).now();
  diag_array.status = status_vec_;

  if (!diag_pub_) {
    diag_pub_ = node->create_publisher<diagnostic_msgs::msg::DiagnosticArray>(
        data.topic_name + "/diagnostics", 1);
  }
  diag_pub_->publish(diag_array);
  return 0;
}

}  // namespace system_stats
}  // namespace mw