#pragma once

#include <glog/logging.h>
#include <pthread.h>
#include <signal.h>

#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "utils/jetsonpower.h"
#include "monitor/monitor_item.h"

namespace mw {
namespace system_stats {

class JtopMonitor : public MonitorItem {
 public:
  explicit JtopMonitor(mw::proto::SystemStatsConfig &cfg)
      : MonitorItem(cfg) {
    jetson_lib_ = JetsonPowerLib::Instance();
  }
  int32_t Start() override;
  int32_t RunOnce(OutputData &msg) override;
  int32_t Stop() override {
    stop_ = true;
    int kill_ret = pthread_kill(thread_->native_handle(), SIGKILL);
    if (kill_ret == ESRCH) {
      LOG(ERROR) << "the specified thread does not exist or has been "
                    "terminated !!!\n ";
    } else if (kill_ret == EINVAL) {
      LOG(ERROR) << "the signal is not valid!!!\n";
    }

    // if (thread_->joinable()) {
    //   thread_->join();
    // }
    return 0;
  }
  const std::string Name() const override { return "JtopMonitor"; }

 private:
  void update_gpu_info();
  void update_power();
  void update_emc();
  void update_profile();
  void update_sensors();
  void UpdateGpuInfo();
  void insert_gpu_info(OutputData &msg);
  void get_virtual_temp();
  std::shared_ptr<JetsonPowerLib> jetson_lib_;
  std::shared_ptr<std::mutex> mutex_ = std::make_shared<std::mutex>();
  std::unique_ptr<std::thread> thread_;
  OutputGpuInfo gpu_info_;
  OutputCpuTemp cpu_temp_;
  bool stop_{false};
};

}  // namespace system_stats
}  // namespace mw
