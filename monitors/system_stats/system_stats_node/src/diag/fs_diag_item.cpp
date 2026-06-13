#include "fs_diag_item.h"

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>

#include "diag/diag_item.h"
#include "diagnostic_msgs/msg/key_value.hpp"

namespace mw {
namespace system_stats {

int32_t FsDiagItem::Start(const mw::proto::SystemStatsDiagConfig &cfg) {
  int ret = -1;
  for (const auto &fs_cfg : cfg.fs()) {
    fs_cfg_[fs_cfg.mount_point()] =
        FsDiagItemConfig{fs_cfg.warn(), fs_cfg.error()};
    ret = 0;
  }
  return ret;
}

int32_t FsDiagItem::Stop() { return 0; }

int32_t FsDiagItem::Diagnose(
    const OutputData &data,
    std::vector<diagnostic_msgs::msg::DiagnosticStatus> &status_vec) {
  for (const auto &fs_info : data.filesystem_infos) {
    if (fs_cfg_.find(fs_info.mount_point) == fs_cfg_.end()) {
      continue;
    }
    diagnose_fs(fs_info, status_vec);
  }
  return 0;
}

void FsDiagItem::diagnose_fs(
    const OutputFileSystemStat &data,
    std::vector<diagnostic_msgs::msg::DiagnosticStatus> &status_vec) {
  auto &mount_point = data.mount_point;
  auto &fs_cfg = fs_cfg_[mount_point];
  diagnostic_msgs::msg::DiagnosticStatus status_msg;
  status_msg.name = "File System";
  if (data.used_percent > fs_cfg.error) {
    status_msg.level = diagnostic_msgs::msg::DiagnosticStatus::ERROR;
  } else if (data.used_percent > fs_cfg.warn) {
    status_msg.level = diagnostic_msgs::msg::DiagnosticStatus::WARN;
  }
  status_msg.values.push_back(
      diagnostic_msgs::build<diagnostic_msgs::msg::KeyValue>()
          .key("mount_point")
          .value(mount_point));
  status_msg.values.push_back(
      diagnostic_msgs::build<diagnostic_msgs::msg::KeyValue>()
          .key("value")
          .value(std::to_string(data.used_percent)));
  status_msg.values.push_back(
      diagnostic_msgs::build<diagnostic_msgs::msg::KeyValue>()
          .key("expected_value")
          .value(std::to_string(fs_cfg.warn)));
  status_msg.values.push_back(
      diagnostic_msgs::build<diagnostic_msgs::msg::KeyValue>()
          .key("unit")
          .value("%"));
  status_vec.push_back(status_msg);
}

REGISTER_DIAG_ITEM_CLASS(FsDiagItem);

}  // namespace system_stats
}  // namespace mw