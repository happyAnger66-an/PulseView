#include <iostream>

#include "stats_output.h"
#include "system_stats_interfaces/msg/system_stats.hpp"

namespace mw {
namespace system_stats {
class Ros2MsgOutput : public StatsOutput {
 public:
  Ros2MsgOutput();

  ~Ros2MsgOutput() = default;

  int32_t Output(const OutputData &data) override;

 private:
  std::shared_ptr<system_stats_interfaces::msg::SystemStats> msg_;
  std::shared_ptr<rclcpp::Publisher<system_stats_interfaces::msg::SystemStats>> pub_;
  int64_t count_ = 0;
};

}  // namespace system_stats
}  // namespace mw