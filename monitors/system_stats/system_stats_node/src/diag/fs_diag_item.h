#pragma once
#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>

#include "diag/diag_item.h"

namespace mw {
namespace system_stats {

class FsDiagItem : public DiagItem {
 public:
  FsDiagItem() : DiagItem() {}
  ~FsDiagItem() = default;

  int32_t Start(const mw::proto::SystemStatsDiagConfig &cfg) override;

  int32_t Stop() override;

  int32_t Diagnose(
      const OutputData &data,
      std::vector<diagnostic_msgs::msg::DiagnosticStatus> &status_vec) override;

 private:
  struct FsDiagItemConfig {
    float warn = 0.0;
    float error = 0.0;
  };
  std::unordered_map<std::string, FsDiagItemConfig> fs_cfg_;
  void diagnose_fs(const OutputFileSystemStat &data,
                   std::vector<diagnostic_msgs::msg::DiagnosticStatus> &status_vec);
};

}  // namespace system_stats
}  // namespace mw