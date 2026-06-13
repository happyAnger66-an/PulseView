#pragma once

#include <boost/algorithm/string.hpp>
#include <memory>

#include "cpu_monitor.h"
#include "mem_monitor.h"
#include "monitor_item.h"
#include "node_latency_monitor.h"
#include "fs_monitor.h"
#include "softirq_monitor.h"
#include "node_monitor.h"
#include "net_monitor.h"
#include "cpu_temp_monitor.h"
#include "gpu_monitor.h"
#include "jtop_monitor.h"

namespace mw {
namespace system_stats {
class MonitorItemFactory {
 public:
  static std::unique_ptr<MonitorItem> Create(
      const std::string &name, mw::proto::SystemStatsConfig &cfg) {
    std::string lower_name(name);
    std::transform(name.begin(), name.end(), lower_name.begin(), ::tolower);
    return CreateMonitorItem(MonitorItem::GetItemTypeByName(lower_name), cfg);
  }

 private:
  static std::unique_ptr<MonitorItem> CreateMonitorItem(
      MonitorItem::ItemType type, mw::proto::SystemStatsConfig &cfg) {
    switch (type) {
      case MonitorItem::ItemType::ItemCpu:
        return std::make_unique<CpuMonitor>(cfg);
        break;
      case MonitorItem::ItemType::ItemMem:
        return std::make_unique<MemMonitor>(cfg);
        break;
      case MonitorItem::ItemType::ItemFs:
        return std::make_unique<FsMonitor>(cfg);
        break;
      case MonitorItem::ItemType::ItemNodeLatency:
        return std::make_unique<NodeLatencyMonitor>(cfg);
        break;
      case MonitorItem::ItemType::ItemSoftIrq:
        return std::make_unique<SoftIrqMonitor>(cfg);
        break;
      case MonitorItem::ItemType::ItemNode:
        return std::make_unique<NodeMonitor>(cfg);
        break;
      case MonitorItem::ItemType::ItemNet:
        return std::make_unique<NetMonitor>(cfg);
        break;
      case MonitorItem::ItemType::ItemCpuTemp:
        return std::make_unique<CpuTempMonitor>(cfg);
        break;
      case MonitorItem::ItemType::ItemGpu:
        return std::make_unique<GpuMonitor>(cfg);
        break;
      case MonitorItem::ItemType::ItemJtop:
        return std::make_unique<JtopMonitor>(cfg);
        break;
      default:
        LOG(ERROR) << "unkown monitor item: [ " << static_cast<int>(type)
                   << " ]";
        break;
    }

    return nullptr;
  }
};

}  // namespace system_stats
}  // namespace mw
