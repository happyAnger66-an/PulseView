#pragma once

#include <mw_base/macros.h>

#include <fstream>
#include <memory>
#include <string>
#include <vector>

#include "diag/stats_diag.h"
#include "monitor/monitor_item.h"
#include "outputs/stats_output.h"
#include "rclcpp/rclcpp.hpp"
#include "system_stats_protos/system_stats.pb.h"

namespace mw {
namespace system_stats {

class SystemStatsNode : public rclcpp::Node {
 public:
  explicit SystemStatsNode(const std::string &config_file,
                           const std::string &node_name)
      : Node(node_name){};
  ~SystemStatsNode();

  virtual bool Init();

 private:
  rclcpp::TimerBase::SharedPtr timer_;

  std::vector<std::unique_ptr<MonitorItem>> all_monitor_items_;
  std::string topic_name_;
  int64_t timestamps_ns_ = 0;

  std::ofstream pub_msg_file_;
  std::unique_ptr<StatsDiag> diag_;

  // mw::proto::VehicleMode* vehicle_mode_ns_;

  void write_pb_msg(const mw::proto::SystemStats &msg);
  void OnTimer();
  void MonitorSlave();

  uint32_t get_timer_interval(uint32_t cfg_interval);
  DISALLOW_COPY_AND_ASSIGN(SystemStatsNode);
};

}  // namespace system_stats
}  // namespace mw
