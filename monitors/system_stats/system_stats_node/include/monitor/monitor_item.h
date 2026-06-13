#pragma once

#include <string>
#include <unordered_map>

#include "system_stats_protos/system_stats_config.pb.h"
#include "outputs/output_datas.h"

namespace mw {
namespace system_stats {

constexpr int KNsToS = 1000 * 1000 * 1000;

class MonitorItem {
 public:
  explicit MonitorItem(const mw::proto::SystemStatsConfig &cfg) : cfg_(cfg) {}
  virtual ~MonitorItem() {}
  enum class ItemType {
    ItemCpu,
    ItemCpuTemp,
    ItemGpu,
    ItemGpuJetson,
    ItemJtop,
    ItemMem,
    ItemFs,
    ItemNet,
    ItemNode,
    ItemSocket,
    ItemTopic,
    ItemSubLat,
    ItemStatus,
    ItemSoftIrq,
    ItemUdp,
    ItemNodeLatency,
    ItemThreads,
    ItemMax,
  };

  static ItemType GetItemTypeByName(const std::string &name) {
    static std::unordered_map<std::string, ItemType> item_map = {
        {"cpu", ItemType::ItemCpu},
        {"cpu_temp", ItemType::ItemCpuTemp},
        {"gpu", ItemType::ItemGpu},
        {"gpu_jetson", ItemType::ItemGpuJetson},
        {"mem", ItemType::ItemMem},
        {"fs", ItemType::ItemFs},
        {"net", ItemType::ItemNet},
        {"node", ItemType::ItemNode},
        {"connection", ItemType::ItemSocket},
        {"topic", ItemType::ItemTopic},
        {"sub_latency", ItemType::ItemSubLat},
        {"status", ItemType::ItemStatus},
        {"softirq", ItemType::ItemSoftIrq},
        {"udp", ItemType::ItemUdp},
        {"node_latency", ItemType::ItemNodeLatency},
        {"threads", ItemType::ItemThreads},
        {"jtop", ItemType::ItemJtop}};

    auto find_iter = item_map.find(name);
    if (find_iter != item_map.end()) {
      return (*find_iter).second;
    }

    return ItemType::ItemMax;
  }

  virtual int32_t Start() = 0;
  virtual int32_t RunOnce(OutputData &output_data) = 0;
  virtual int32_t Stop() = 0;
  virtual const std::string Name() const { return "MonitorItem"; }

 protected:
  mw::proto::SystemStatsConfig cfg_;
};

}  // namespace system_stats
}  // namespace mw
