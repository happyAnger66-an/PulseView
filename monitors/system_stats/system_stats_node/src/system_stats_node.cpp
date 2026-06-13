#include "system_stats_node.h"

#include <absl/strings/str_format.h>
#include <fcntl.h>
#include <glog/logging.h>
#include <mw_base/base_dir.h>
#include <mw_base/now.h>

#include <chrono>
#include <string>
#include <unordered_map>

#include "monitor/monitor_factory.h"
#include "mw_common/onboard_config.h"
#include "mw_startup/startup.h"
#include "outputs/output_datas.h"
#include "outputs/stats_output.h"
#include "system_stats_protos/system_stats_config.pb.h"
#include "system_stats_protos/system_stats_diag_config.pb.h"

using namespace std::chrono_literals;

namespace mw {

namespace system_stats {
constexpr char kSystemStatsMasterTopic[] = "/system_stats";
constexpr char kSystemStatsEnvHz[] = "system_stats_hz";

uint32_t SystemStatsNode::get_timer_interval(uint32_t cfg_interval) {
  char *env_hz = getenv(kSystemStatsEnvHz);
  int interval = 3000;
  if (env_hz == nullptr) {
    interval = cfg_interval;
  } else {
    int i_interval = atoi(env_hz);
    if (i_interval <= 0) {
      interval = cfg_interval;
    } else {
      interval = 1000 / i_interval;
    }
  }

  LOG(INFO) << "use timer interval:" << interval << "ms";
  return interval;
}

bool SystemStatsNode::Init() {
  // load node config
  mw::proto::SystemStatsConfig config;
  std::string mon_cfg_file = "system_stats/system_stats_config";
  std::string diag_cfg_file = "system_stats/system_stats_diag_config";

  std::string board_name = mw_startup::GetBoardName();
  if (board_name.empty()) {
    LOG(ERROR) << "get board name failed !!!";
    topic_name_ = "/system_stats";
  } else {
    mon_cfg_file =
        absl::StrFormat("system_stats/system_stats_config.%s", board_name);
    diag_cfg_file =
        absl::StrFormat("system_stats/system_stats_diag_config.%s", board_name);
    topic_name_ = absl::StrFormat("/%s/system_stats", board_name);
  }

  LOG(INFO) << "use mon_cfg_file: " << mon_cfg_file;
  LOG(INFO) << "use diag_cfg_file: " << diag_cfg_file;
  LOG(INFO) << "use topic_name: " << topic_name_;

  config = mw_common::CreateConfig<mw::proto::SystemStatsConfig>(mon_cfg_file);
  LOG(INFO) << "load config  "
            << "\n"
            << config.DebugString();

  for (const auto &monitor_item : config.mon_cfg().monitor_item()) {
    all_monitor_items_.emplace_back(
        MonitorItemFactory::Create(monitor_item, config));
  }

  for (auto it = all_monitor_items_.begin(); it != all_monitor_items_.end();) {
    LOG(INFO) << "item " << (*it)->Name().c_str() << " start begin !!!";
    if (!(*it)->Start()) {
      LOG(WARNING) << "item " << (*it)->Name().c_str() << " start success !!!";
      it++;
    } else {
      LOG(ERROR) << "item " << (*it)->Name().c_str() << " start failed !!!";
      it = all_monitor_items_.erase(it);
    }
  }

  mw::proto::SystemStatsDiagConfig diag_config;
  diag_config =
      mw_common::CreateConfig<mw::proto::SystemStatsDiagConfig>(diag_cfg_file);
  LOG(INFO) << "load diag config  "
            << "\n"
            << diag_config.DebugString();

  diag_ = std::make_unique<StatsDiag>(diag_config);

  OutputManager::instance()->Init(config, (rclcpp::Node *)this);

  // register all timer
  timer_ =
      create_wall_timer(3000ms, std::bind(&SystemStatsNode::OnTimer, this));

  return true;
}

SystemStatsNode::~SystemStatsNode() {
  for (auto &monitor_item : all_monitor_items_) {
    LOG(INFO) << "item " << monitor_item->Name().c_str() << " stop begin !!!";
    if (!monitor_item->Stop()) {
      LOG(WARNING) << "item " << monitor_item->Name().c_str()
                   << " stop success !!!";
    } else {
      LOG(INFO) << "item " << monitor_item->Name().c_str()
                << " stop failed !!!";
    }
  }
}

void SystemStatsNode::write_pb_msg(const mw::proto::SystemStats &msg) {
  int32_t msg_size = msg.ByteSize();
  pub_msg_file_.write(reinterpret_cast<char *>(&msg_size), sizeof(msg_size));

  std::string m_data;
  msg.SerializeToString(&m_data);
  pub_msg_file_ << m_data;
}

void SystemStatsNode::OnTimer() {
  std::cerr << "OnTimer" << std::endl;
  mw::proto::SystemStats system_info_msg;
  auto &output_data = OutputManager::instance()->GetOutputData();
  for (const auto &monitor_item : all_monitor_items_) {
    std::cerr << "monitor_item " << monitor_item->Name().c_str() << std::endl;
    monitor_item->RunOnce(output_data);
  }

  output_data.topic_name = topic_name_;
  diag_->Run((rclcpp::Node *)this, output_data);
  OutputManager::instance()->Output();

  write_pb_msg(system_info_msg);
}

}  // namespace system_stats
}  // namespace mw
