#include "monitor/jtop_monitor.h"

#include <dlfcn.h>
#include <nvml.h>

#include <filesystem>
#include <iostream>
#include <string>
#include <unordered_map>

#include "utils/jetsonpower.h"
#include "utils/proc_file.h"
#include "utils/proc_stat.h"
#include "utils/process.h"

namespace mw {
namespace system_stats {

// /etc/nvpower/libjetsonpower/jetsonpower_t234.conf

constexpr static char kCpuTempStr[] = "CPU";
constexpr static char kGpuTempStr[] = "GPU";
constexpr static char kHwmonDir[] = "/sys/class/hwmon/";
constexpr static char kThermalDir[] = "/sys/devices/virtual/thermal/";

template <typename Func>
Func Load(void *handle, const std::string &name) {
  void *symbol = dlsym(handle, name.c_str());
  auto *err = dlerror();
  CHECK(err == nullptr) << err;
  return reinterpret_cast<Func>(symbol);
}

int32_t JtopMonitor::Start() {
  if (!jetson_lib_->is_loaded) {
    return -1;
  }

  thread_ = std::make_unique<std::thread>([this]() {
    pthread_setname_np(pthread_self(), "JtopMonitorThread");
    while (!stop_) {
      UpdateGpuInfo();
      std::this_thread::sleep_for(std::chrono::seconds(3));
    }
  });
  return 0;
}

static std::string_view trim_view(std::string_view s) {
  auto start = s.find_first_not_of(" \t\n\r\f\v");
  if (start == std::string_view::npos) return "";

  auto end = s.find_last_not_of(" \t\n\r\f\v");
  return s.substr(start, end - start + 1);
}

// 转换为string
static std::string trim(std::string_view s) {
  return std::string(trim_view(s));
}

static std::string read_file(const std::filesystem::path &path) {
  std::ifstream file(path);
  if (!file.is_open()) {
    return "";
  }
  std::string content;
  file >> content;
  return content;
}

void JtopMonitor::get_virtual_temp() {
  {
    std::lock_guard<std::mutex> scoped_lock(*mutex_);
    gpu_info_.gpu_sensor_infos.clear();
  }

  std::vector<std::string> thermal_dirs;
  for (const auto &entry : std::filesystem::directory_iterator(kThermalDir)) {
    if (entry.is_directory() && entry.path().filename().string().find(
                                    "thermal_") != std::string::npos) {
      thermal_dirs.push_back(entry.path().string());
    }
  }

  int idx = 1;
  std::unordered_map<std::string, int> temp_map;
  for (const auto &thermal_dir : thermal_dirs) {
    std::string name;
    auto type_path = thermal_dir + "/type";
    auto temp_path = thermal_dir + "/temp";
    if (std::filesystem::exists(type_path) &&
        std::filesystem::exists(temp_path)) {
      auto type = read_file(type_path);
      auto temp = read_file(temp_path);
      if (type.find("-") != std::string::npos) {
        name = type.substr(0, type.find("-"));
      } else {
        name = type;
      }
      name = trim(name);

      if (temp_map.find(name) == temp_map.end()) {
        temp_map[name] = 0;
      } else {
        idx = temp_map[name] + 1;
        temp_map[name] = idx;
        name = name + " " + std::to_string(idx);
      }

      temp = trim(temp);
      float temp_value = std::stof(temp);

      {
        std::lock_guard<std::mutex> scoped_lock(*mutex_);
        OutputGpuSensorInfo sensor_info;
        sensor_info.name = name;
        sensor_info.temperature = temp_value / 1000.0;
        gpu_info_.gpu_sensor_infos.push_back(sensor_info);
      }
    }
  }
}

void JtopMonitor::update_gpu_info() {
  nvmlReturn_t ret;

  void *libnvidia = dlopen("libnvidia-ml.so.1", RTLD_LAZY);

  if (libnvidia) {
    ret = Load<decltype(&nvmlInit)>(libnvidia, "nvmlInit")();
    if (NVML_SUCCESS != ret) {
      LOG(ERROR) << "Failed to initialize NVML ("
                 << Load<decltype(&nvmlErrorString)>(libnvidia,
                                                     "nvmlErrorString")(ret)
                 << ")";
      return;
    }

    uint32_t device_count;
    ret = Load<decltype(&nvmlDeviceGetCount)>(
        libnvidia, "nvmlDeviceGetCount")(&device_count);
    // ret = nvmlDeviceGetCount(&device_count);
    if (NVML_SUCCESS != ret) {
      LOG(ERROR) << "Failed to query device count ("
                 << Load<decltype(&nvmlErrorString)>(libnvidia,
                                                     "nvmlErrorString")(ret)
                 << ")";
      return;
    }

    for (uint32_t i = 0; i < device_count; i++) {
      nvmlDevice_t device;
      ret = Load<decltype(&nvmlDeviceGetHandleByIndex)>(
          libnvidia, "nvmlDeviceGetHandleByIndex")(i, &device);
      // ret = nvmlDeviceGetHandleByIndex(i, &device);
      if (NVML_SUCCESS != ret) {
        LOG(ERROR) << "Failed to get handle for device " << i << "("
                   << Load<decltype(&nvmlErrorString)>(libnvidia,
                                                       "nvmlErrorString")(ret)
                   << ")";
        continue;
      }

      char name[NVML_DEVICE_NAME_BUFFER_SIZE];

      ret = Load<decltype(&nvmlDeviceGetName)>(libnvidia, "nvmlDeviceGetName")(
          device, name, NVML_DEVICE_NAME_BUFFER_SIZE);
      // ret = nvmlDeviceGetName(device, name, NVML_DEVICE_NAME_BUFFER_SIZE);
      if (NVML_SUCCESS != ret) {
        LOG(ERROR) << "Failed to get name of device " << i << "("
                   << Load<decltype(&nvmlErrorString)>(libnvidia,
                                                       "nvmlErrorString")(ret)
                   << ")";
        continue;
      }

      nvmlUtilization_t utilization;
      ret = Load<decltype(&nvmlDeviceGetUtilizationRates)>(
          libnvidia, "nvmlDeviceGetUtilizationRates")(device, &utilization);
      // ret = nvmlDeviceGetUtilizationRates(device, &utilization);
      if (NVML_SUCCESS != ret) {
        LOG(ERROR) << "device " << i << "Failed to get Gpu usage: "
                   << Load<decltype(&nvmlErrorString)>(libnvidia,
                                                       "nvmlErrorString")(ret);
        continue;
      }

      {
        std::lock_guard<std::mutex> scoped_lock(*mutex_);
        gpu_info_.name = name;
        gpu_info_.gpu_usage = utilization.gpu;
      }
    }
  }
}

void JtopMonitor::UpdateGpuInfo() {
  update_gpu_info();
  update_sensors();
  update_power();
  update_profile();
}

void JtopMonitor::update_sensors() {
  get_virtual_temp();
  return;
}

void JtopMonitor::update_power() {
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
  for (int i = 0; i < nums; i++) {
    name = names[i];

    int in_power = jetson_lib_->JetsonPower_rail_get_power(name);
    int avg_power = jetson_lib_->JetsonPower_rail_get_avg_power(name);
    int warn_power = jetson_lib_->JetsonPower_rail_get_warn_current(name);
    int crit_power = jetson_lib_->JetsonPower_rail_get_crit_current(name);

    OutputGpuPowerInfo power_info;
    power_info.name = std::string(name);
    power_info.inst_power = in_power / 1000.0;
    power_info.avg_power = avg_power / 1000.0;
    {
      std::lock_guard<std::mutex> scoped_lock(*mutex_);
      gpu_info_.gpu_power_infos.push_back(power_info);
    }
  }

  return;
}

void JtopMonitor::update_profile() {
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

void JtopMonitor::update_emc() {
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
}

void JtopMonitor::insert_gpu_info(OutputData &msg) {
  auto &one_gpu_info = msg.AddGpuInfo();
  auto &emc_info = one_gpu_info.gpu_emc_info;

  std::lock_guard<std::mutex> scoped_lock(*mutex_);
  one_gpu_info.gpu_usage = gpu_info_.gpu_usage;

  // emc info
  auto &gpu_emc_info = gpu_info_.gpu_emc_info;
  emc_info.emc_freq_mhz = gpu_emc_info.emc_freq_mhz;
  emc_info.emc_load = gpu_emc_info.emc_load;

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
  }

  // sensor info, cpu temperature, gpu temperature
  for (auto &gpu_sensor_info : gpu_info_.gpu_sensor_infos) {
    OutputGpuSensorInfo sensor_info;
    sensor_info.name = gpu_sensor_info.name;
    sensor_info.temperature = gpu_sensor_info.temperature;
    one_gpu_info.gpu_sensor_infos.push_back(sensor_info);
  }
}

int32_t JtopMonitor::RunOnce(mw::system_stats::OutputData &msg) {
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
