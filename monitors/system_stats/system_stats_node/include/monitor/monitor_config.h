#pragma once

#include <pthread.h>
#include <map>
#include <memory>
#include <string>

#include <ros/ros.h>
#include <boost/noncopyable.hpp>
#include "mw_stats_protos/system_stats_protos/system_stats_config.pb.h"

namespace mw {
namespace system_stats {
class MonitorConfig ::private boost::noncopyable {
  static MonitorConfig& Instance() {
    pthread_once(&once, &MonitorConfig::init);
    return *monitor_config_instance;
  }

  static void init() { monitor_config_instance = new MonitorConfig(); }
  static pthread_once_t once;
  static MonitorConfig* monitor_config_instance;
  mw::SystemStatsConfig monitor_cfg_;
};
}  // namespace system_stats
}  // namespace mw