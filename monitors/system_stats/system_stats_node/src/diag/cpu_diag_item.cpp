#include "cpu_diag_item.h"

#include "diagnostic_msgs/msg/key_value.hpp"

namespace mw {
namespace system_stats {

int32_t CpuDiagItem::Start(const mw::proto::SystemStatsDiagConfig &cfg) {
  int32_t ret = -1;
  cfg_ = cfg;

  if (cfg.has_system_cpu()) {
    system_cpu_.warn = cfg.system_cpu().warn();
    system_cpu_.error = cfg.system_cpu().error();
    ret = 0;
  }

  if (cfg.has_default_cpu()) {
    default_cpu_.warn = cfg.default_cpu().warn();
    default_cpu_.error = cfg.default_cpu().error();
    ret = 0;
  }

  if (cfg.node_size() > 0) {
    auto &nodes_cfg = cfg.node();

    float warn = 0.0;
    float error = 0.0;
    for (auto &node_cfg : nodes_cfg) {
      if (!node_cfg.has_cpu()) {
        continue;
      }

      if (node_cfg.cpu().warn() > 0.0) {
        warn = node_cfg.cpu().warn();
      }

      if (node_cfg.cpu().error() > 0.0) {
        error = node_cfg.cpu().error();
      }

      CpuDiagItemConfig node_cpu_config;
      node_cpu_config.warn = warn;
      node_cpu_config.error = error;
      nodes_cpu_.emplace(node_cfg.name(), node_cpu_config);
      ret = 0;
    }
  }

  return ret;
}

int32_t CpuDiagItem::Stop() { return 0; }

void CpuDiagItem::node_defaul_cpu_diag(
    const OutputNodeInfo &node_cpu,
    std::vector<diagnostic_msgs::msg::DiagnosticStatus> &status_vec) {
  if(node_cpu.cpu_used_percent <= default_cpu_.warn) {
    return;
  }
  const auto &name = node_cpu.name;
  diagnostic_msgs::msg::DiagnosticStatus status;
  status.name = "Node Default CPU";
  if (node_cpu.cpu_used_percent > default_cpu_.error) {
    status.level = diagnostic_msgs::msg::DiagnosticStatus::ERROR;

  } else if (node_cpu.cpu_used_percent > default_cpu_.warn) {
    status.level = diagnostic_msgs::msg::DiagnosticStatus::WARN;
  }
  status.values.push_back(
      diagnostic_msgs::build<diagnostic_msgs::msg::KeyValue>()
          .key("name")
          .value(name));
  status.values.push_back(
      diagnostic_msgs::build<diagnostic_msgs::msg::KeyValue>()
          .key("value")
          .value(std::to_string(node_cpu.cpu_used_percent)));
  status.values.push_back(
      diagnostic_msgs::build<diagnostic_msgs::msg::KeyValue>()
          .key("expected_value")
          .value(std::to_string(default_cpu_.warn)));
  status.values.push_back(
      diagnostic_msgs::build<diagnostic_msgs::msg::KeyValue>()
          .key("unit")
          .value("%"));
  status_vec.push_back(status);

  return;
}

bool CpuDiagItem::node_cpu_diag(
    const OutputNodeInfo &node_cpu,
    std::vector<diagnostic_msgs::msg::DiagnosticStatus> &status_vec) {
  const auto &name = node_cpu.name;

  diagnostic_msgs::msg::DiagnosticStatus status;
  if (nodes_cpu_.find(name) != nodes_cpu_.end()) {
    auto &node_cpu_config = nodes_cpu_[name];
    if (node_cpu.cpu_used_percent < node_cpu_config.warn) {
      return false;
    }
    status.name = "Node CPU";
    if (node_cpu.cpu_used_percent > node_cpu_config.error) {
      status.level = diagnostic_msgs::msg::DiagnosticStatus::ERROR;
    } else if (node_cpu.cpu_used_percent > node_cpu_config.warn) {
      status.level = diagnostic_msgs::msg::DiagnosticStatus::WARN;
    }
    status.values.push_back(
        diagnostic_msgs::build<diagnostic_msgs::msg::KeyValue>()
            .key("name")
            .value(name));
    status.values.push_back(
        diagnostic_msgs::build<diagnostic_msgs::msg::KeyValue>()
            .key("value")
            .value(std::to_string(node_cpu.cpu_used_percent)));
    status.values.push_back(
        diagnostic_msgs::build<diagnostic_msgs::msg::KeyValue>()
            .key("expected_value")
            .value(std::to_string(node_cpu_config.warn)));
    status.values.push_back(
        diagnostic_msgs::build<diagnostic_msgs::msg::KeyValue>()
            .key("unit")
            .value("%"));
    status_vec.push_back(status);

    return true;
  }

  return false;
}

int32_t CpuDiagItem::Diagnose(
    const OutputData &data,
    std::vector<diagnostic_msgs::msg::DiagnosticStatus> &status_vec) {
  if (system_cpu_.warn > 0.0 && system_cpu_.error > 0.0) {
    diagnostic_msgs::msg::DiagnosticStatus status;
    status.name = "System CPU";
    if (data.cpu.cpu_percent > system_cpu_.error) {
      status.level = diagnostic_msgs::msg::DiagnosticStatus::ERROR;
    } else if (data.cpu.cpu_percent > system_cpu_.warn) {
      status.level = diagnostic_msgs::msg::DiagnosticStatus::WARN;
    }
    status.values.push_back(
        diagnostic_msgs::build<diagnostic_msgs::msg::KeyValue>()
            .key("value")
            .value(std::to_string(data.cpu.cpu_percent)));
    status.values.push_back(
        diagnostic_msgs::build<diagnostic_msgs::msg::KeyValue>()
            .key("expected_value")
            .value(std::to_string(system_cpu_.warn)));
    status.values.push_back(
        diagnostic_msgs::build<diagnostic_msgs::msg::KeyValue>()
            .key("unit")
            .value("%"));
    status_vec.push_back(status);
  }

  if (default_cpu_.warn > 0.0 && default_cpu_.error > 0.0) {
    for (auto &node_cpu : data.node_infos) {
      if (node_cpu_diag(node_cpu, status_vec)) {
        continue;
      }

      node_defaul_cpu_diag(node_cpu, status_vec);
    }
  }

  return 0;
}

REGISTER_DIAG_ITEM_CLASS(CpuDiagItem)

}  // namespace system_stats
}  // namespace mw