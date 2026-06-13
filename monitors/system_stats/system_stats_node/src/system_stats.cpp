#include <mw_base/base_dir.h>
#include <mw_base/now.h>
#include <mw_proto_util/proto_io.h>
#include <mw_startup/startup.h>

#include <absl/strings/str_format.h>
#include <glog/logging.h>

#include "system_stats_node.h"

bool GetNodeConfigFile(std::string &cfg_file) {
  cfg_file = mw_base::GetBaseDirPath(mw_base::BaseDir::kConfigDir) +
             "/node/system_stats/system_stats_node" + ".pb.conf";
  LOG(INFO) << "cfg_file: " << cfg_file;

  return true;
}

int main(int argc, char **argv) {
  google::ParseCommandLineFlags(&argc, &argv, true);
  google::InitGoogleLogging(argv[0]);
  google::InstallFailureSignalHandler();

  // get module config
  using namespace mw::system_stats;

  std::string config_file;
  if (!GetNodeConfigFile(config_file)) {
    LOG(ERROR) << "mw_monitor get node_cfg_file failed !!!";
    return -1;
  }

  rclcpp::init(argc, argv);
  std::string board_name = mw_startup::GetBoardName();
  std::string node_name = absl::StrFormat("system_stats_%s", board_name);
  auto system_stats_node = std::make_shared<mw::system_stats::SystemStatsNode>(config_file, node_name);
  system_stats_node->Init();
  rclcpp::spin(system_stats_node);
  rclcpp::shutdown();

}
