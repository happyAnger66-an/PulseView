#include <glog/logging.h>
#include "mw_base/base_dir.h"

#include "monitor/monitor_config.h"
#include "mw_proto_util/proto_io.h"

namespace mw {
namespace system_stats {

pthread_once_t MonitorConfig::once = PTHREAD_ONCE_INIT;
MonitorConfig* MonitorConfig::monitor_config_instance = nullptr;

MonitorConfig::MonitorConfig() {
  // get bring config
  std::string config_path = mw_base::GetBaseDirPath(mw_base::BaseDir::kConfigDir) +
                            "/config/default/onboard/system_stats.pb.conf";
  LOG(INFO) << "system_stats config read";
  if (!mw_proto_util::ReadTextProtoFile(config_path, &monitor_cfg_)) {
    LOG(WARNING) << "system_stats failed to load config: " << config_path;
  }
}

}  // namespace system_stats
}  // namespace mw
