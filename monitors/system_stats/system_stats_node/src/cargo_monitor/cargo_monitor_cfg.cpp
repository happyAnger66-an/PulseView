#include <mw_base/base_dir.h>
#include <glog/logging.h>

#include "mw_comm/onboard_config.h"
#include "mw_monitor/mw_monitor.h"

namespace mw {
namespace system_stats {

bool SystemStatsGetConfig(mw::proto::SystemStatsConfig &config) {
  config = mw_comm::CreateConfig<mw::proto::SystemStatsConfig>(
      "system_stats_config");
  return true;
}

}  // namespace system_stats
}  // namespace mw
