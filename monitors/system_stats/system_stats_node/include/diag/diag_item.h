#pragma once

#include <system_stats_protos/system_stats_diag_config.pb.h>

#include <cstdint>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>

#include "diagnostic_msgs/msg/diagnostic_status.hpp"
#include "outputs/output_datas.h"
#include "system_stats_protos/system_stats_diag_config.pb.h"

namespace mw {
namespace system_stats {
using NodeTopicKey = std::pair<std::string, std::string>;

struct StringPairHash {
  std::size_t operator()(const NodeTopicKey &p) const {
    // 组合两个字符串的哈希值
    auto h1 = std::hash<std::string>{}(p.first);
    auto h2 = std::hash<std::string>{}(p.second);

    // 常用的哈希组合方法
    // 方法A：异或组合（注意：x ^ x = 0，可能冲突）
    // return h1 ^ h2;

    // 方法B：boost-like 组合（更少冲突）
    // return h1 ^ (h2 << 1);

    // 方法C：更复杂的组合
    return h1 ^ (h2 + 0x9e3779b9 + (h1 << 6) + (h1 >> 2));
  }
};

// 方法2：定义相等比较器
struct StringPairEqual {
  bool operator()(const NodeTopicKey &a, const NodeTopicKey &b) const {
    return a.first == b.first && a.second == b.second;
  }
};
class DiagItem {
 public:
  DiagItem() = default;
  virtual ~DiagItem() = default;

  virtual int32_t Start(const mw::proto::SystemStatsDiagConfig &cfg) = 0;

  virtual int32_t Stop() = 0;

  virtual int32_t Diagnose(
      const OutputData &data,
      std::vector<diagnostic_msgs::msg::DiagnosticStatus> &status_vec) = 0;

 protected:
  mw::proto::SystemStatsDiagConfig cfg_;
};

class DiagItemFactory {
 private:
  struct DiagItemCreator {
    std::function<std::unique_ptr<DiagItem>()> createSimple;
  };

  std::unordered_map<std::string, DiagItemCreator> creators;

 public:
  template <typename T>
  void registerDiagItem(const std::string &format) {
    std::cerr << "registerDiagItem: format " << format << std::endl;
    creators[format] = DiagItemCreator{
        []() -> std::unique_ptr<DiagItem> { return std::make_unique<T>(); },
    };
  }

  static DiagItemFactory &getInstance() {
    static DiagItemFactory instance;
    return instance;
  }

  void CreateDiagItems(
      std::unordered_map<std::string, std::unique_ptr<DiagItem>> &diag_items) {
    for (auto &creator : creators) {
      diag_items[creator.first] = creator.second.createSimple();
    }
  }

  std::unique_ptr<DiagItem> createDiagItem(const std::string &format) {
    auto it = creators.find(format);
    if (it != creators.end()) {
      return it->second.createSimple();
    }
    std::cerr << "DiagItemFactory::createDiagItem: format " << format
              << " not found" << std::endl;
    return nullptr;
  }
};

template <typename Derived, const char *Name>
class DiagItemAutoRegister {
 public:
  DiagItemAutoRegister() {
    DiagItemFactory::getInstance().registerDiagItem<Derived>(Name);
  }

  // 防止拷贝
  DiagItemAutoRegister(const DiagItemAutoRegister &) = delete;
  DiagItemAutoRegister &operator=(const DiagItemAutoRegister &) = delete;
};

// 辅助宏
#define STRINGIFY(x) #x
#define REGISTER_DIAG_ITEM_CLASS(DerivedClass)                              \
  constexpr const char __##DerivedClass##_name[] = STRINGIFY(DerivedClass); \
  inline const DiagItemAutoRegister<DerivedClass, __##DerivedClass##_name>  \
      __##DerivedClass##_registrar;

}  // namespace system_stats
}  // namespace mw