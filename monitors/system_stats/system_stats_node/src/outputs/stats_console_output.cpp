#include "outputs/stats_console_output.h"

#include <iostream>

namespace mw {
namespace system_stats {

int32_t ConsoleOutput::Output(const OutputData &output_data) {
  std::cout << output_data.cpu << std::endl;
  std::cout << output_data.mem << std::endl;

  for (const auto &cpu_info : output_data.cpu_infos) {
    std::cout << cpu_info << std::endl;
  }

  std::cout << output_data.mem_detail << std::endl;

  for (const auto &node_info : output_data.node_infos) {
    std::cout << node_info << std::endl;
  }

  for (const auto &node_pub_info : output_data.node_pub_infos) {
    std::cout << node_pub_info << std::endl;
  }

  for (const auto &fs_info : output_data.filesystem_infos) {
    std::cout << fs_info << std::endl;
  }

  for (const auto &softirq_info : output_data.softirq_infos) {
    std::cout << softirq_info << std::endl;
  }

  for (const auto &cpu_irq_info : output_data.cpu_irq_infos) {
    std::cout << cpu_irq_info << std::endl;
  }

  for (const auto &pub_info : output_data.node_pub_infos) {
    std::cout << pub_info << std::endl;
  }

  for (const auto &node_sub_info : output_data.node_sub_infos) {
    std::cout << node_sub_info << std::endl;
  }

  for (const auto &cpu_temp_info : output_data.cpu_temp_infos) {
    std::cout << cpu_temp_info << std::endl;
  }

  for (const auto &gpu_info : output_data.gpu_infos) {
    std::cout << gpu_info << std::endl;
  }

  for (const auto &gpu_process : output_data.gpu_processes) {
    std::cout << gpu_process << std::endl;
  }
  return 0;
}

REGISTER_OUTPUT_CLASS(ConsoleOutput)

}  // namespace system_stats
}  // namespace mw