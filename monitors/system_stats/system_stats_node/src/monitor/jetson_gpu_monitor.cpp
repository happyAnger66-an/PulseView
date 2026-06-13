#include "monitor/jetson_gpu_monitor.h"

#include <iostream>
#include <string>

#include "utils/jetsonpower.h"
#include "utils/proc_file.h"
#include "utils/proc_stat.h"
#include "utils/process.h"

namespace mw {
namespace system_stats {

// /etc/nvpower/libjetsonpower/jetsonpower_t234.conf

constexpr static char kGpuLoadFile[] = "/sys/devices/gpu.0/load";
constexpr static char kGpuCurFreqFile[] =
    "/sys/kernel/debug/clk/gpusysclk/clk_rate";
constexpr static char kEmcCurFreqFile[] = "/sys/kernel/debug/clk/emc/clk_rate";
constexpr static char kEmcCurLoadFile[] =
    "/sys/kernel/actmon_avg_activity/mc_all";
constexpr static char kCpuTempStr[] = "CPU";
constexpr static char kGpuTempStr[] = "GPU";

int32_t JetsonGpuMonitor::Start() {
  if (!jetson_lib_->is_loaded) {
    return -1;
  }
  thread_ = std::make_unique<std::thread>([this]() {
    pthread_setname_np(pthread_self(), "JetsonGpuThread");
    while (!stop_) {
      UpdateGpuInfo();
      std::this_thread::sleep_for(std::chrono::seconds(3));
    }
  });
  return 0;
}

void JetsonGpuMonitor::UpdateGpuInfo() {
  float gpu_load = jetson_lib_->JetsonPower_igpu_get_load(0);
  float gpu_freq_mhz = jetson_lib_->JetsonPower_igpu_get_cur_freq(0);

  {
    std::lock_guard<std::mutex> scoped_lock(*mutex_);
    gpu_info_.gpu_freq_mhz = gpu_freq_mhz / 1000.0;
    gpu_info_.gpu_usage = gpu_load;
  }

  update_emc();
  update_power();
  update_profile();
  update_sensors();
}

void JetsonGpuMonitor::update_sensors() {
  char **names = jetson_lib_->JetsonPower_sensor_get_names();
  if (names == nullptr) {
    return;
  }

  int nums = jetson_lib_->JetsonPower_sensor_get_nums();
  if (nums <= 0) {
    return;
  }
  {
    std::lock_guard<std::mutex> scoped_lock(*mutex_);
    gpu_info_.gpu_sensor_infos.clear();
  }
  int i = 0;
  char *name = nullptr;
  for (; i < nums; i++) {
    name = names[i];
    int temp = jetson_lib_->JetsonPower_sensor_get_temp(name);
    if (temp > 0) {
      float temp = jetson_lib_->JetsonPower_sensor_get_temp(name) / 1000.0;
      float high =
          jetson_lib_->JetsonPower_sensor_get_sw_throt_temp(name) / 1000.0;
      float crit =
          jetson_lib_->JetsonPower_sensor_get_sw_shutdown_temp(name) / 1000.0;

      OutputGpuSensorInfo sensor_info;
      sensor_info.name = std::string(name);
      sensor_info.temperature = temp;
      sensor_info.exist_cpu_temp = false;
      sensor_info.exist_gpu_temp = false;

      {
        std::lock_guard<std::mutex> scoped_lock(*mutex_);
        gpu_info_.update_count = {true, 0};
        std::size_t found = std::string(name).find(kCpuTempStr);
        if (found != std::string::npos) {
          sensor_info.exist_cpu_temp = true;
          cpu_temp_.description = name;
          cpu_temp_.temperature = temp;
          cpu_temp_.high = high;
          cpu_temp_.crit = crit;
        }

        found = std::string(name).find(kGpuTempStr);
        if (found != std::string::npos) {
          sensor_info.exist_gpu_temp = true;
          gpu_info_.temperature = temp;
          gpu_info_.temperature_slow = high;
          gpu_info_.temperature_down = crit;
        }

        gpu_info_.gpu_sensor_infos.push_back(sensor_info);
      }
    }
  }

  return;
}

void JetsonGpuMonitor::update_power() {
  char **names = jetson_lib_->JetsonPower_rail_get_names();
  if (names == nullptr) {
    return;
  }

  int nums = jetson_lib_->JetsonPower_rail_get_nums();
  if (nums <= 0) {
    return;
  }
  {
    std::lock_guard<std::mutex> scoped_lock(*mutex_);
    gpu_info_.gpu_power_infos.clear();
  }
  int i = 0;
  char *name = nullptr;
  for (; i < nums; i++) {
    name = names[i];

    int in_power = jetson_lib_->JetsonPower_rail_get_power(name);
    int avg_power = jetson_lib_->JetsonPower_rail_get_avg_power(name);
    int warn_power = jetson_lib_->JetsonPower_rail_get_warn_current(name);
    int crit_power = jetson_lib_->JetsonPower_rail_get_crit_current(name);

    OutputGpuPowerInfo power_info;
    power_info.name = std::string(name);
    power_info.inst_power = in_power;
    power_info.avg_power = avg_power;
    power_info.exist_gpu = false;
    std::size_t found = std::string(name).find(kGpuTempStr);
    if (found != std::string::npos) {
      power_info.exist_gpu = true;
      std::lock_guard<std::mutex> scoped_lock(*mutex_);
      gpu_info_.pwr_usage = in_power;
      gpu_info_.pwr_cap = crit_power;
    }
    {
      std::lock_guard<std::mutex> scoped_lock(*mutex_);
      gpu_info_.gpu_power_infos.push_back(power_info);
    }
  }

  return;
}

void JetsonGpuMonitor::update_profile() {
  char **names = jetson_lib_->JetsonPower_fan_get_names();
  if (names == nullptr) {
    return;
  }

  int nums = jetson_lib_->JetsonPower_fan_get_nums();
  if (nums <= 0) {
    return;
  }

  {
    std::lock_guard<std::mutex> scoped_lock(*mutex_);
    gpu_info_.gpu_fan_infos.clear();
  }
  int i = 0;
  char *name = nullptr;
  for (; i < nums; i++) {
    name = names[i];
    int pwm = jetson_lib_->JetsonPower_fan_get_pwm(i + 1);
    int speed = jetson_lib_->JetsonPower_fan_get_speed(i + 1);
    char *profile = jetson_lib_->JetsonPower_fan_get_profile(i + 1);
    OutputGpuFanInfo fan_info;
    fan_info.name = std::string(name);
    fan_info.profile = std::string(profile);
    fan_info.pwm = pwm * 100.0 / 255;
    fan_info.rpm = speed;
    {
      std::lock_guard<std::mutex> scoped_lock(*mutex_);
      gpu_info_.gpu_fan_infos.push_back(fan_info);
    }
  }

  return;
}

void JetsonGpuMonitor::update_emc() {
  float emc_load = jetson_lib_->JetsonPower_emc_get_load();
  float emc_freq_mhz = jetson_lib_->JetsonPower_emc_get_cur_freq();

  float total_ram = jetson_lib_->JetsonPower_emc_get_mem_size();
  float free_ram = jetson_lib_->JetsonPower_emc_get_mem_free_size();
  float buffers_ram = jetson_lib_->JetsonPower_emc_get_buffers_size();
  float cached_ram = jetson_lib_->JetsonPower_emc_get_cached_size();
  float total_swap = jetson_lib_->JetsonPower_emc_get_swap_size();
  float free_swap = jetson_lib_->JetsonPower_emc_get_swap_free_size();

  std::lock_guard<std::mutex> scoped_lock(*mutex_);
  auto &emc_info = gpu_info_.gpu_emc_info;
  emc_info.emc_freq_mhz = emc_freq_mhz / 1000.0;
  emc_info.emc_load = emc_load;
  if (total_ram > 0) {
    float mem_used = total_ram - free_ram - buffers_ram - cached_ram;
    float mem_load = mem_used / total_ram * 100.0;
    // emc_info->set_emc_mem_total(total_ram);
    // emc_info->set_emc_mem_used(mem_used);
    // emc_info->set_emc_mem_load(mem_load);

    // gpu_info.set_mem_used_percent(mem_load);
    // gpu_info.set_mem_total(total_ram);

    emc_info.emc_mem_total = total_ram;
    emc_info.emc_mem_used = mem_used;
    emc_info.emc_mem_load = mem_load;

    gpu_info_.mem_used_percent = mem_load;
    gpu_info_.mem_total = total_ram;
  }

  if (total_swap > 0) {
    int swap_used = total_swap - free_swap;
    float swap_load = swap_used / total_swap * 100.0;
    // emc_info->set_emc_mem_swap_total(total_swap);
    // emc_info->set_emc_mem_swap_used(swap_used);
    // emc_info->set_emc_mem_swap_load(swap_load);

    emc_info.emc_mem_swap_total = total_swap;
    emc_info.emc_mem_swap_used = swap_used;
    emc_info.emc_mem_swap_load = swap_load;
  }
}

void JetsonGpuMonitor::insert_gpu_info(OutputData &msg) {
  auto &one_gpu_info = msg.AddGpuInfo();
  auto &emc_info = one_gpu_info.gpu_emc_info;

  std::lock_guard<std::mutex> scoped_lock(*mutex_);
  one_gpu_info.gpu_freq_mhz = gpu_info_.gpu_freq_mhz;
  one_gpu_info.gpu_usage = gpu_info_.gpu_usage;
  one_gpu_info.mem_used_percent = gpu_info_.mem_used_percent;
  one_gpu_info.mem_total = gpu_info_.mem_total;

  // emc info
  auto &gpu_emc_info = gpu_info_.gpu_emc_info;
  emc_info.emc_freq_mhz = gpu_emc_info.emc_freq_mhz;
  emc_info.emc_load = gpu_emc_info.emc_load;
  emc_info.emc_mem_total = gpu_emc_info.emc_mem_total;
  emc_info.emc_mem_used = gpu_emc_info.emc_mem_used;
  emc_info.emc_mem_load = gpu_emc_info.emc_mem_load;
  emc_info.emc_mem_swap_total = gpu_emc_info.emc_mem_swap_total;
  emc_info.emc_mem_swap_used = gpu_emc_info.emc_mem_swap_used;
  emc_info.emc_mem_swap_load = gpu_emc_info.emc_mem_swap_load;

  // fan info
  for (auto &gpu_fan_info : gpu_info_.gpu_fan_infos) {
    OutputGpuFanInfo fan_info;
    fan_info.name = gpu_fan_info.name;
    fan_info.profile = gpu_fan_info.profile;
    fan_info.pwm = gpu_fan_info.pwm;
    fan_info.rpm = gpu_fan_info.rpm;
    one_gpu_info.gpu_fan_infos.push_back(fan_info);
  }

  // power info
  for (auto &gpu_power_info : gpu_info_.gpu_power_infos) {
    OutputGpuPowerInfo power_info;
    power_info.name = gpu_power_info.name;
    power_info.inst_power = gpu_power_info.inst_power;
    power_info.avg_power = gpu_power_info.avg_power;
    one_gpu_info.gpu_power_infos.push_back(power_info);
    if (gpu_power_info.exist_gpu) {
      one_gpu_info.pwr_usage = gpu_info_.pwr_usage;
      one_gpu_info.pwr_cap = gpu_info_.pwr_cap;
    }
  }

  // sensor info, cpu temperature, gpu temperature
  for (auto &gpu_sensor_info : gpu_info_.gpu_sensor_infos) {
    if (gpu_sensor_info.exist_gpu_temp) {
      OutputGpuSensorInfo sensor_info;
      sensor_info.name = gpu_sensor_info.name;
      sensor_info.temperature = gpu_sensor_info.temperature;
      sensor_info.exist_cpu_temp = gpu_sensor_info.exist_cpu_temp;
      sensor_info.exist_gpu_temp = gpu_sensor_info.exist_gpu_temp;
      one_gpu_info.gpu_sensor_infos.push_back(sensor_info);

      if (gpu_sensor_info.exist_cpu_temp) {
        OutputCpuTemp cpu_temperature;
        cpu_temperature.description = cpu_temp_.description;
        cpu_temperature.temperature = cpu_temp_.temperature;
        cpu_temperature.high = cpu_temp_.high;
        cpu_temperature.crit = cpu_temp_.crit;
        msg.cpu_temp_infos.push_back(cpu_temperature);
      }

      if (gpu_sensor_info.exist_gpu_temp) {
        one_gpu_info.temperature = gpu_info_.temperature;
        one_gpu_info.temperature_slow = gpu_info_.temperature_slow;
        one_gpu_info.temperature_down = gpu_info_.temperature_down;
      }
    }
  }
}

int32_t JetsonGpuMonitor::RunOnce(mw::system_stats::OutputData &msg) {
  ProcFile gpu_load_file(kGpuLoadFile);
  ProcFile gpu_cur_freq_file(kGpuCurFreqFile);

  // if the update failed more than 10 times, print an error message
  {
    std::lock_guard<std::mutex> scoped_lock(*mutex_);
    if (!gpu_info_.update_count.first) {
      gpu_info_.update_count.second += 1;
      if (gpu_info_.update_count.second > 10) {
        gpu_info_.update_count.second = 0;
        LOG_EVERY_N(ERROR, 100) << "failed to update gpu info!!!";
      }
    }
    gpu_info_.update_count.first = false;
  }

  // insert data
  insert_gpu_info(msg);

  return 0;
}

}  // namespace system_stats
}  // namespace mw
