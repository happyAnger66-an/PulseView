#include "mem_diag_item.h"

#include "diagnostic_msgs/msg/key_value.hpp"

namespace mw {
namespace system_stats {

int32_t MemDiagItem::Start(const mw::proto::SystemStatsDiagConfig &cfg) {
  int32_t ret = -1;
  cfg_ = cfg;

  if (cfg.has_system_mem()) {
    system_mem_.warn = cfg.system_mem().warn();
    system_mem_.error = cfg.system_mem().error();
    ret = 0;
  }

  if (cfg.has_default_mem()) {
    default_mem_.warn = cfg.default_mem().warn();
    default_mem_.error = cfg.default_mem().error();
    ret = 0;
  }

  if (cfg.node_size() > 0) {
    auto &nodes_cfg = cfg.node();

    float warn = 0.0;
    float error = 0.0;
    for (auto &node_cfg : nodes_cfg) {
      if (!node_cfg.has_mem()) {
        continue;
      }

      if (node_cfg.mem().warn() > 0.0) {
        warn = node_cfg.mem().warn();
      }

      if (node_cfg.mem().error() > 0.0) {
        error = node_cfg.mem().error();
      }

      MemDiagItemConfig node_mem_config;
      node_mem_config.warn = warn;
      node_mem_config.error = error;
      nodes_mem_.emplace(node_cfg.name(), node_mem_config);
      ret = 0;
    }
  }

  return ret;
}

int32_t MemDiagItem::Stop() { return 0; }

void MemDiagItem::node_defaul_mem_diag(
    const OutputNodeInfo &node_mem,
    std::vector<diagnostic_msgs::msg::DiagnosticStatus> &status_vec) {
  if(node_mem.mem_used_percent <= default_mem_.warn) {
    return;
  }
  const auto &name = node_mem.name;
  diagnostic_msgs::msg::DiagnosticStatus status;
  status.name = "Node Default Mem";
  if (node_mem.mem_used_percent > default_mem_.error) {
    status.level = diagnostic_msgs::msg::DiagnosticStatus::ERROR;

  } else if (node_mem.mem_used_percent > default_mem_.warn) {
    status.level = diagnostic_msgs::msg::DiagnosticStatus::WARN;
  }
  status.values.push_back(
      diagnostic_msgs::build<diagnostic_msgs::msg::KeyValue>()
          .key("name")
          .value(name));
  status.values.push_back(
      diagnostic_msgs::build<diagnostic_msgs::msg::KeyValue>()
          .key("value")
          .value(std::to_string(node_mem.mem_used_percent)));
  status.values.push_back(
      diagnostic_msgs::build<diagnostic_msgs::msg::KeyValue>()
          .key("expected_value")
          .value(std::to_string(default_mem_.warn)));
  status.values.push_back(
      diagnostic_msgs::build<diagnostic_msgs::msg::KeyValue>()
          .key("unit")
          .value("%"));
  status_vec.push_back(status);

  return;
}

bool MemDiagItem::node_mem_diag(
    const OutputNodeInfo &node_mem,
    std::vector<diagnostic_msgs::msg::DiagnosticStatus> &status_vec) {
  const auto &name = node_mem.name;

  diagnostic_msgs::msg::DiagnosticStatus status;
  if (nodes_mem_.find(name) != nodes_mem_.end()) {
    auto &node_mem_config = nodes_mem_[name];
    if (node_mem.mem_used_percent < node_mem_config.warn) {
      return false;
    }
    status.name = "Node Mem";
    if (node_mem.mem_used_percent > node_mem_config.error) {
      status.level = diagnostic_msgs::msg::DiagnosticStatus::ERROR;
    } else if (node_mem.mem_used_percent > node_mem_config.warn) {
      status.level = diagnostic_msgs::msg::DiagnosticStatus::WARN;
    }
    status.values.push_back(
        diagnostic_msgs::build<diagnostic_msgs::msg::KeyValue>()
            .key("name")
            .value(name));
    status.values.push_back(
        diagnostic_msgs::build<diagnostic_msgs::msg::KeyValue>()
            .key("value")
            .value(std::to_string(node_mem.mem_used_percent)));
    status.values.push_back(
        diagnostic_msgs::build<diagnostic_msgs::msg::KeyValue>()
            .key("expected_value")
            .value(std::to_string(node_mem_config.warn)));
    status.values.push_back(
        diagnostic_msgs::build<diagnostic_msgs::msg::KeyValue>()
            .key("unit")
            .value("%"));
    status_vec.push_back(status);

    return true;
  }

  return false;
}

int32_t MemDiagItem::Diagnose(
    const OutputData &data,
    std::vector<diagnostic_msgs::msg::DiagnosticStatus> &status_vec) {
  if (system_mem_.warn > 0.0 && system_mem_.error > 0.0) {
    diagnostic_msgs::msg::DiagnosticStatus status;
    status.name = "System Mem";
    if (data.mem.used_percent > system_mem_.error) {
      status.level = diagnostic_msgs::msg::DiagnosticStatus::ERROR;
    } else if (data.mem.used_percent > system_mem_.warn) {
      status.level = diagnostic_msgs::msg::DiagnosticStatus::WARN;
    }
    status.values.push_back(
        diagnostic_msgs::build<diagnostic_msgs::msg::KeyValue>()
            .key("value")
            .value(std::to_string(data.mem.used_percent)));
    status.values.push_back(
        diagnostic_msgs::build<diagnostic_msgs::msg::KeyValue>()
            .key("expected_value")
            .value(std::to_string(system_mem_.warn)));
    status.values.push_back(
        diagnostic_msgs::build<diagnostic_msgs::msg::KeyValue>()
            .key("unit")
            .value("%"));
    status_vec.push_back(status);
  }

  if (default_mem_.warn > 0.0 && default_mem_.error > 0.0) {
    for (auto &node_mem : data.node_infos) {
      if (node_mem_diag(node_mem, status_vec)) {
        continue;
      }

      node_defaul_mem_diag(node_mem, status_vec);
    }
  }

  return 0;
}

REGISTER_DIAG_ITEM_CLASS(MemDiagItem)

}  // namespace system_stats
}  // namespace mw
