#pragma once
#include <mw_base/macros.h>

#include <functional>
#include <memory>
#include <unordered_map>

#include "output_datas.h"
#include "system_stats_protos/system_stats_config.pb.h"
#include <rclcpp/rclcpp.hpp>

namespace mw {
namespace system_stats {

class StatsOutput {
 public:
  StatsOutput() = default;

  void SetNode(rclcpp::Node *node) { node_ = node; }

  virtual ~StatsOutput() = default;

  virtual int32_t Output(const OutputData &data) = 0;

  protected:
    rclcpp::Node *node_;
};

class StatsOutputFactory {
 private:
  struct OutputCreator {
    std::function<std::unique_ptr<StatsOutput>()> createSimple;
  };

  std::unordered_map<std::string, OutputCreator> creators;

 public:
  template <typename T>
  void registerOutput(const std::string &format) {
    std::cerr << "registerOutput: format " << format << std::endl;
    creators[format] = OutputCreator{
        []() -> std::unique_ptr<StatsOutput> { return std::make_unique<T>(); },
    };
  }

  static StatsOutputFactory &getInstance() {
    static StatsOutputFactory instance;
    return instance;
  }

  std::unique_ptr<StatsOutput> createOutput(const std::string &format) {
    auto it = creators.find(format);
    if (it != creators.end()) {
      return it->second.createSimple();
    }
    std::cerr << "StatsOutputFactory::createOutput: format " << format
              << " not found" << std::endl;
    return nullptr;
  }
};

template <typename Derived, const char *Name>
class OutputAutoRegister {
 public:
  OutputAutoRegister() {
    StatsOutputFactory::getInstance().registerOutput<Derived>(Name);
  }

  // 防止拷贝
  OutputAutoRegister(const OutputAutoRegister &) = delete;
  OutputAutoRegister &operator=(const OutputAutoRegister &) = delete;
};

// 辅助宏
#define STRINGIFY(x) #x
#define REGISTER_OUTPUT_CLASS(DerivedClass)                                 \
  constexpr const char __##DerivedClass##_name[] = STRINGIFY(DerivedClass); \
  inline const OutputAutoRegister<DerivedClass, __##DerivedClass##_name>    \
      __##DerivedClass##_registrar;

class OutputManager {
 public:
  int32_t Output() {
    for (auto &output : outputs) {
      output.second->Output(output_data_);
    }

    output_data_.clear();
    return 0;
  }

  void Init(const mw::proto::SystemStatsConfig &config,
            rclcpp::Node *node) {
    for (const auto &output_name : config.mon_cfg().output()) {
      outputs.emplace(
          output_name,
          StatsOutputFactory::getInstance().createOutput(output_name));
      outputs[output_name]->SetNode(node);
    }
    
  }

  OutputData &GetOutputData() { return output_data_; }

 private:
  OutputData output_data_{};
  std::unordered_map<std::string, std::unique_ptr<StatsOutput>> outputs;
  DECLARE_SINGLETON(OutputManager)
};

}  // namespace system_stats
}  // namespace mw