#include "outputs/stats_ros2msg_output.h"

#include <iostream>

#include "system_stats_interfaces/msg/system_stats.hpp"

namespace mw {
namespace system_stats {

Ros2MsgOutput::Ros2MsgOutput() {
  msg_ = std::make_unique<system_stats_interfaces::msg::SystemStats>();
}

int32_t Ros2MsgOutput::Output(const OutputData &output_data) {
  //  std::cout << "ros2 msg output:" << std::endl;
  count_++;
  msg_->cpu_used_percent = output_data.cpu.cpu_percent;
  msg_->mem_free_size = output_data.mem.avail_size;
  msg_->mem_total_size = output_data.mem.total_size;
  msg_->mem_used_percent = output_data.mem.used_percent;

  msg_->header.stamp = rclcpp::Clock(RCL_ROS_TIME).now();
  msg_->header.frame_id = std::to_string(count_);

  auto &cpu_stats = msg_->cpu_stats;
  cpu_stats.clear();
  cpu_stats.reserve(output_data.cpu_infos.size());
  for (const auto &cpu_info : output_data.cpu_infos) {
    system_stats_interfaces::msg::CpuStat cpu_stat;
    cpu_stat.cpu_name = cpu_info.cpu_name;
    cpu_stat.cpu_percent = cpu_info.cpu_percent;
    cpu_stat.cpu_user_percent = cpu_info.cpu_user_percent;
    cpu_stat.cpu_sys_percent = cpu_info.cpu_sys_percent;
    cpu_stat.cpu_idle_percent = cpu_info.cpu_idle_percent;
    cpu_stat.cpu_wa_percent = cpu_info.cpu_wa_percent;
    cpu_stat.cpu_si_percent = cpu_info.cpu_si_percent;
    cpu_stat.cpu_hi_percent = cpu_info.cpu_hi_percent;
    cpu_stat.cpu_ni_percent = cpu_info.cpu_ni_percent;
    cpu_stats.push_back(cpu_stat);
  }

  auto &mem_detail_stat = msg_->mem_detail_stat;
  auto &mem_detail = output_data.mem_detail;
  mem_detail_stat.swap_cached = mem_detail.swap_cached;
  mem_detail_stat.active = mem_detail.active;
  mem_detail_stat.inactive = mem_detail.inactive;
  mem_detail_stat.active_anon = mem_detail.active_anon;
  mem_detail_stat.inactive_anon = mem_detail.inactive_anon;
  mem_detail_stat.active_file = mem_detail.active_file;
  mem_detail_stat.dirty = mem_detail.dirty;
  mem_detail_stat.writeback = mem_detail.writeback;
  mem_detail_stat.anon_pages = mem_detail.writeback;
  mem_detail_stat.mapped = mem_detail.writeback;
  mem_detail_stat.kreclaimable = mem_detail.kreclaimable;
  mem_detail_stat.sreclaimable = mem_detail.sreclaimable;
  mem_detail_stat.sunreclaim = mem_detail.sunreclaim;

  auto &filesystem_stat = msg_->filesystem_stats;
  filesystem_stat.clear();
  filesystem_stat.reserve(output_data.filesystem_infos.size());
  for (const auto &fs_info : output_data.filesystem_infos) {
    system_stats_interfaces::msg::FileSystemStat fs_stat;
    fs_stat.type = fs_info.type;
    fs_stat.total = fs_info.total;
    fs_stat.used = fs_info.used;
    fs_stat.free = fs_info.free;
    fs_stat.used_percent = fs_info.used_percent;
    fs_stat.mount_point = fs_info.mount_point;
    fs_stat.read_req_speed = fs_info.read_req_speed;
    fs_stat.write_req_speed = fs_info.write_req_speed;
    fs_stat.read_kb_speed = fs_info.read_kb_speed;
    fs_stat.write_kb_speed = fs_info.write_kb_speed;
    fs_stat.read_wait = fs_info.read_wait;
    fs_stat.write_wait = fs_info.write_wait;
    fs_stat.aqu_sz = fs_info.aqu_sz;
    fs_stat.util = fs_info.util;
    filesystem_stat.push_back(fs_stat);
  }

  auto &softirq_stats = msg_->softirq_stats;
  softirq_stats.clear();
  softirq_stats.reserve(output_data.softirq_infos.size());
  for (const auto &softirq_info : output_data.softirq_infos) {
    system_stats_interfaces::msg::SoftIrqStat softirq_stat;
    softirq_stat.cpu = softirq_info.cpu;
    softirq_stat.hi = softirq_info.hi;
    softirq_stat.timer = softirq_info.timer;
    softirq_stat.net_tx = softirq_info.net_tx;
    softirq_stat.net_rx = softirq_info.net_rx;
    softirq_stat.block = softirq_info.block;
    softirq_stat.irq_poll = softirq_info.irq_poll;
    softirq_stat.tasklet = softirq_info.tasklet;
    softirq_stat.sched = softirq_info.sched;
    softirq_stat.hrtimer = softirq_info.hrtimer;
    softirq_stat.rcu = softirq_info.rcu;
    softirq_stats.push_back(softirq_stat);
  }

  auto &cpu_irq_stats = msg_->cpu_irq_stats;
  cpu_irq_stats.clear();
  cpu_irq_stats.reserve(output_data.cpu_irq_infos.size());
  for (const auto &cpu_irq_info : output_data.cpu_irq_infos) {
    system_stats_interfaces::msg::CpuIrqStat cpu_irq_stat;
    cpu_irq_stat.cpu = cpu_irq_info.cpu;
    cpu_irq_stat.irq = cpu_irq_info.irq;
    cpu_irq_stat.speed = cpu_irq_info.speed;
    cpu_irq_stats.push_back(cpu_irq_stat);
  }

  auto &node_stats = msg_->proc_stats;
  node_stats.clear();
  node_stats.reserve(output_data.node_infos.size());
  for (const auto &node_info : output_data.node_infos) {
    system_stats_interfaces::msg::ProcStat node_stat;
    node_stat.name = node_info.name;
    node_stat.pid = node_info.pid;
    node_stat.status = node_info.status;
    node_stat.cpu_used_percent = node_info.cpu_used_percent;
    node_stat.cpu_user_percent = node_info.cpu_user_percent;
    node_stat.cpu_sys_percent = node_info.cpu_sys_percent;
    node_stat.mem_used_percent = node_info.mem_used_percent;
    node_stat.io_delay = node_info.io_delay;
    node_stat.minflt = node_info.minflt;
    node_stat.majflt = node_info.majflt;
    node_stat.processor = node_info.processor;
    node_stat.threads = node_info.threads;
    node_stat.rss_shmem = node_info.rss_shmem;
    node_stat.vm_swap = node_info.vm_swap;
    node_stat.voluntary_ctxt_switches = node_info.voluntary_ctxt_switches;
    node_stat.nonvoluntary_ctxt_switches = node_info.nonvoluntary_ctxt_switches;
    node_stat.sched_policy = node_info.sched_policy;
    node_stat.sched_prio = node_info.sched_prio;
    node_stat.sched_run_time = node_info.sched_run_time;
    node_stat.sched_wait_time = node_info.sched_wait_time;
    node_stat.sched_run_cnts = node_info.sched_run_cnts;
    node_stat.cpu_wait_percent = node_info.cpu_wait_percent;
    node_stat.read_kb_speed = node_info.read_kb_speed;
    node_stat.write_kb_speed = node_info.write_kb_speed;
    node_stat.read_mbytes = node_info.read_mbytes;
    node_stat.write_mbytes = node_info.write_mbytes;
    node_stats.push_back(node_stat);
  }

  auto &net_infos = msg_->net_stats;
  net_infos.clear();
  net_infos.reserve(output_data.net_infos.size());
  for (const auto &net_info : output_data.net_infos) {
    system_stats_interfaces::msg::NetStat net_stat;
    net_stat.name = net_info.name;
    net_stat.status = net_info.status;
    net_stat.mtu = net_info.mtu;
    net_stat.send_rate = net_info.send_rate;
    net_stat.rcv_rate = net_info.rcv_rate;
    net_stat.send_pkts_rate = net_info.send_pkts_rate;
    net_stat.rcv_pkts_rate = net_info.rcv_pkts_rate;
    net_infos.push_back(net_stat);
  }

  auto &gpu_stats = msg_->gpu_stats;
  gpu_stats.clear();
  gpu_stats.reserve(output_data.gpu_infos.size());
  for (const auto &gpu_info : output_data.gpu_infos) {
    system_stats_interfaces::msg::GpuStat gpu_stat;
    auto &power_stats = gpu_stat.power_stats;
    power_stats.clear();
    power_stats.reserve(gpu_info.gpu_power_infos.size());
    for (const auto &gpu_power_info : gpu_info.gpu_power_infos) {
      system_stats_interfaces::msg::PowerStat power_stat;
      power_stat.name = gpu_power_info.name;
      power_stat.inst_power = gpu_power_info.inst_power;
      power_stat.avg_power = gpu_power_info.avg_power;
      power_stats.push_back(power_stat);
    }

    auto &sensor_stats = gpu_stat.sensor_stats;
    sensor_stats.clear();
    sensor_stats.reserve(gpu_info.gpu_sensor_infos.size());
    for (const auto &gpu_sensor_info : gpu_info.gpu_sensor_infos) {
      system_stats_interfaces::msg::SensorStat sensor_stat;
      sensor_stat.name = gpu_sensor_info.name;
      sensor_stat.temperature = gpu_sensor_info.temperature;
      sensor_stats.push_back(sensor_stat);
    }

    gpu_stat.name = gpu_info.name;
    gpu_stat.gpu_usage = gpu_info.gpu_usage;
    gpu_stats.push_back(gpu_stat);
  }

  auto &node_pub_stats = msg_->node_pub_stats;
  node_pub_stats.clear();
  node_pub_stats.reserve(output_data.node_pub_infos.size());
  for (const auto &node_pub_info : output_data.node_pub_infos) {
    system_stats_interfaces::msg::NodePubStat node_pub_stat;
    node_pub_stat.node = node_pub_info.node;
    node_pub_stat.topic = node_pub_info.topic;
    node_pub_stat.hz = node_pub_info.hz;
    node_pub_stat.min_delta = node_pub_info.min_delta;
    node_pub_stat.max_delta = node_pub_info.max_delta;
    node_pub_stat.avg_delta = node_pub_info.avg_delta;
    node_pub_stat.min_delta_ts = node_pub_info.min_delta_ts;
    node_pub_stat.max_delta_ts = node_pub_info.max_delta_ts;

    node_pub_stat.min_proc_delta = node_pub_info.min_proc_delta;
    node_pub_stat.max_proc_delta = node_pub_info.max_proc_delta;
    node_pub_stat.avg_proc_delta = node_pub_info.avg_proc_delta;
    node_pub_stat.min_proc_delta_ts = node_pub_info.min_proc_delta_ts;
    node_pub_stat.max_proc_delta_ts = node_pub_info.max_proc_delta_ts;

    node_pub_stat.data_ts = node_pub_info.data_ts;
    node_pub_stats.push_back(node_pub_stat);
  }

  auto &sub_stats = msg_->node_sub_stats;
  sub_stats.clear();
  sub_stats.reserve(output_data.node_sub_infos.size());
  for (const auto &node_sub_info : output_data.node_sub_infos) {
    system_stats_interfaces::msg::NodeSubStat node_sub_stat;
    node_sub_stat.node = node_sub_info.node;
    node_sub_stat.topic = node_sub_info.topic;
    node_sub_stat.hz = node_sub_info.hz;
    node_sub_stat.min_delta = node_sub_info.min_delta;
    node_sub_stat.max_delta = node_sub_info.max_delta;
    node_sub_stat.avg_delta = node_sub_info.avg_delta;
    node_sub_stat.min_delta_ts = node_sub_info.min_delta_ts;
    node_sub_stat.max_delta_ts = node_sub_info.max_delta_ts;

    node_sub_stat.min_proc_delta = node_sub_info.min_proc_delta;
    node_sub_stat.max_proc_delta = node_sub_info.max_proc_delta;
    node_sub_stat.avg_proc_delta = node_sub_info.avg_proc_delta;
    node_sub_stat.min_proc_delta_ts = node_sub_info.min_proc_delta_ts;
    node_sub_stat.max_proc_delta_ts = node_sub_info.max_proc_delta_ts;

    node_sub_stat.min_sched_delta = node_sub_info.min_sched_delta;
    node_sub_stat.max_sched_delta = node_sub_info.max_sched_delta;
    node_sub_stat.avg_sched_delta = node_sub_info.avg_sched_delta;
    node_sub_stat.min_sched_delta_ts = node_sub_info.min_sched_delta_ts;
    node_sub_stat.max_sched_delta_ts = node_sub_info.max_sched_delta_ts;

    node_sub_stat.min_ipc = node_sub_info.min_ipc / 1000.0;
    node_sub_stat.max_ipc = node_sub_info.max_ipc / 1000.0;
    node_sub_stat.avg_ipc = node_sub_info.avg_ipc / 1000.0;
    node_sub_stat.min_ipc_ts = node_sub_info.min_ipc_ts;
    node_sub_stat.max_ipc_ts = node_sub_info.max_ipc_ts;
    node_sub_stat.data_ts = node_sub_info.data_ts;
    sub_stats.push_back(node_sub_stat);
  }

  if (!pub_) {
    pub_ = node_->create_publisher<system_stats_interfaces::msg::SystemStats>(
        output_data.topic_name, 1);
  }

  pub_->publish(*msg_);

  /*
  std::cout << output_data.mem_detail << std::endl;

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
  }*/
  return 0;
}

REGISTER_OUTPUT_CLASS(Ros2MsgOutput)

}  // namespace system_stats
}  // namespace mw
