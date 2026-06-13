#pragma once

#include <iostream>
#include <string>
#include <vector>

struct OutputGpuProcess {
  std::string name = "";
  int32_t pid = 0;
  int32_t mem_used = 0;
  int32_t gpu_used = 0;
  friend std::ostream& operator<<(std::ostream& os,
                                  const OutputGpuProcess& gpu_process) {
    os << "\n";
    os << "name: " << gpu_process.name << "\n";
    os << "pid: " << gpu_process.pid << "\n";
    os << "mem_used: " << gpu_process.mem_used << "MB\n";
    os << "gpu_used: " << gpu_process.gpu_used << "%\n";
    os << "\n";
    return os;
  }
};

struct OutputGpuPowerInfo {
  std::string name = "";
  int32_t inst_power = 0;
  int32_t avg_power = 0;
  friend std::ostream& operator<<(std::ostream& os,
                                  const OutputGpuPowerInfo& gpu_power_info) {
    os << "\n";
    os << "name: " << gpu_power_info.name << "\n";
    os << "inst_power: " << gpu_power_info.inst_power << "W\n";
    os << "avg_power: " << gpu_power_info.avg_power << "W\n";
    os << "\n";
    return os;
  }
};

struct OutputGpuEmcInfo {
  float emc_freq_mhz = 0.0;
  float emc_load = 0.0;
  friend std::ostream& operator<<(std::ostream& os,
                                  const OutputGpuEmcInfo& gpu_emc_info) {
    os << "\n";
    os << "emc_freq_mhz: " << gpu_emc_info.emc_freq_mhz << "MHz\n";
    os << "emc_load: " << gpu_emc_info.emc_load << "\n";
    os << "\n";
    return os;
  }
};

struct OutputGpuFanInfo {
  std::string name = "";
  std::string profile = "";
  float pwm = 0.0;
  int32_t rpm = 0;
  friend std::ostream& operator<<(std::ostream& os,
                                  const OutputGpuFanInfo& gpu_fan_info) {
    os << "\n";
    os << "name: " << gpu_fan_info.name << "\n";
    os << "profile: " << gpu_fan_info.profile << "\n";
    os << "pwm: " << gpu_fan_info.pwm << "\n";
    os << "rpm: " << gpu_fan_info.rpm << "\n";
    os << "\n";
    return os;
  }
};

struct OutputGpuSensorInfo {
  std::string name = "";
  float temperature = 0.0;
  friend std::ostream& operator<<(std::ostream& os,
                                  const OutputGpuSensorInfo& gpu_sensor_info) {
    os << "\n";
    os << "name: " << gpu_sensor_info.name << "\n";
    os << "temperature: " << gpu_sensor_info.temperature << "°C\n";
    os << "\n";
    return os;
  }
};

struct OutputGpuInfo {
  std::string name = "";
  float temperature = 0.0;
  float gpu_usage = 0.0;
  OutputGpuEmcInfo gpu_emc_info;
  std::vector<OutputGpuPowerInfo> gpu_power_infos;
  std::vector<OutputGpuFanInfo> gpu_fan_infos;
  std::vector<OutputGpuSensorInfo> gpu_sensor_infos;
  std::pair<bool, int> update_count = {false, 0};
  friend std::ostream& operator<<(std::ostream& os,
                                  const OutputGpuInfo& gpu_info) {
    os << "\n";
    os << "name: " << gpu_info.name << "\n";
    os << "gpu_usage: " << gpu_info.gpu_usage << "%\n";
    for (const auto& gpu_power_info : gpu_info.gpu_power_infos) {
      os << gpu_power_info << "\n";
    }
    for (const auto& gpu_fan_info : gpu_info.gpu_fan_infos) {
      os << gpu_fan_info << "\n";
    }
    for (const auto& gpu_sensor_info : gpu_info.gpu_sensor_infos) {
      os << gpu_sensor_info << "\n";
    }
    os << "update_count: " << gpu_info.update_count.first << ", "
       << gpu_info.update_count.second << "\n";
    os << "\n";
    return os;
  }
};