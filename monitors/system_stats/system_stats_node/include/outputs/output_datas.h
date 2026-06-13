#pragma once

#include <iostream>
#include <string>
#include <vector>

#include "outputs/output_gpu_data.h"

namespace mw {
namespace system_stats {

struct OutputDataCpu {
  float cpu_percent = 0.0;
  float user_percent = 0.0;
  float sys_percent = 0.0;
  float idle_percent = 0.0;
  float wait_percent = 0.0;
  float ni_percent = 0.0;
  float si_percent = 0.0;
  float hi_percent = 0.0;
  friend std::ostream& operator<<(std::ostream& os, const OutputDataCpu& cpu) {
    os << "\n";
    os << "cpu_info: "
       << "\n";
    os << "cpu_percent: " << cpu.cpu_percent << "%\n";
    os << "user_percent: " << cpu.user_percent << "%\n";
    os << "sys_percent: " << cpu.sys_percent << "%\n";
    os << "idle_percent: " << cpu.idle_percent << "%\n";
    os << "wait_percent: " << cpu.wait_percent << "%\n";
    os << "ni_percent: " << cpu.ni_percent << "%\n";
    os << "si_percent: " << cpu.si_percent << "%\n";
    os << "hi_percent: " << cpu.hi_percent << "%\n";
    os << "\n";
    return os;
  }
};

struct OutputCpuLoad {
  float load_avg_1 = 0.0;
  float load_avg_5 = 0.0;
  float load_avg_15 = 0.0;
  int64_t cs = 0;
  int64_t procs = 0;
  int64_t procs_running = 0;
  int64_t procs_blocked = 0;
};

struct OutputDataMem {
  float free_size = 0.0;
  float used_percent = 0.0;
  float avail_size = 0.0;
  float total_size = 0.0;
  float buffers_size = 0.0;
  float cached_size = 0.0;

  friend std::ostream& operator<<(std::ostream& os, const OutputDataMem& mem) {
    os << "\n";
    os << "mem_info: "
       << "\n";
    os << "free_size: " << mem.free_size << "GB\n";
    os << "used_percent: " << mem.used_percent << "%\n";
    os << "avail_size: " << mem.avail_size << "GB\n";
    os << "total_size: " << mem.total_size << "GB\n";
    os << "buffers_size: " << mem.buffers_size << "GB\n";
    os << "cached_size: " << mem.cached_size << "GB\n";
    os << "\n";
    return os;
  }
};

struct OutputDataMemDetail {
  float swap_cached = 0.0;
  float active = 0.0;  // MB
  float inactive = 0.0;
  float active_anon = 0.0;
  float inactive_anon = 0.0;
  float active_file = 0.0;
  float inactive_file = 0.0;
  float dirty = 0.0;
  float writeback = 0.0;
  float anon_pages = 0.0;
  float mapped = 0.0;
  float kreclaimable = 0.0;
  float sreclaimable = 0.0;
  float sunreclaim = 0.0;
  friend std::ostream& operator<<(std::ostream& os,
                                  const OutputDataMemDetail& mem) {
    os << "\n";
    os << "mem_detail: "
       << "\n";
    os << "swap_cached: " << mem.swap_cached << "MB\n";
    os << "active: " << mem.active << "MB\n";
    os << "inactive: " << mem.inactive << "MB\n";
    os << "active_anon: " << mem.active_anon << "MB\n";
    os << "inactive_anon: " << mem.inactive_anon << "MB\n";
    os << "active_file: " << mem.active_file << "MB\n";
    os << "inactive_file: " << mem.inactive_file << "MB\n";
    os << "dirty: " << mem.dirty << "MB\n";
    os << "writeback: " << mem.writeback << "MB\n";
    os << "anon_pages: " << mem.anon_pages << "MB\n";
    os << "mapped: " << mem.mapped << "MB\n";
    os << "kreclaimable: " << mem.kreclaimable << "MB\n";
    os << "sreclaimable: " << mem.sreclaimable << "MB\n";
    os << "sunreclaim: " << mem.sunreclaim << "MB\n";
    os << "\n";
    return os;
  }
};

struct OutputDataNodePubInfo {
  std::string node = "";
  std::string topic = "";
  float hz = 0.0;
  float min_delta = 0.0;
  float max_delta = 0.0;
  float avg_delta = 0.0;
  int64_t min_delta_ts = 0;
  int64_t max_delta_ts = 0;
  float min_proc_delta = 0.0;
  float max_proc_delta = 0.0;
  float avg_proc_delta = 0.0;
  int64_t min_proc_delta_ts = 0;
  int64_t max_proc_delta_ts = 0;
  int64_t data_ts = 0;
  friend std::ostream& operator<<(std::ostream& os,
                                  const OutputDataNodePubInfo& node_pub_info) {
    os << "\n";
    os << "node: " << node_pub_info.node << "\n";
    os << "topic: " << node_pub_info.topic << "\n";
    os << "hz: " << node_pub_info.hz << "\n";
    os << "min_delta: " << node_pub_info.min_delta << "ms\n";
    os << "max_delta: " << node_pub_info.max_delta << "ms\n";
    os << "avg_delta: " << node_pub_info.avg_delta << "ms\n";
    os << "min_proc_delta: " << node_pub_info.min_proc_delta << "s\n";
    os << "max_proc_delta: " << node_pub_info.max_proc_delta << "s\n";
    os << "avg_proc_delta: " << node_pub_info.avg_proc_delta << "s\n";
    os << "min_proc_delta_ts: " << node_pub_info.min_proc_delta_ts << "\n";
    os << "max_proc_delta_ts: " << node_pub_info.max_proc_delta_ts << "\n";
    os << "data_ts: " << node_pub_info.data_ts << "\n";
    os << "\n";
    return os;
  }
};

struct OutputDataNodeSubInfo {
  std::string node = "";
  std::string topic = "";
  float hz = 0.0;
  float min_delta = 0.0;
  float max_delta = 0.0;
  float avg_delta = 0.0;
  int64_t min_delta_ts = 0;
  int64_t max_delta_ts = 0;
  float min_proc_delta = 0.0;
  float max_proc_delta = 0.0;
  float avg_proc_delta = 0.0;
  int64_t min_proc_delta_ts = 0;
  int64_t max_proc_delta_ts = 0;
  float min_sched_delta = 0.0;
  float max_sched_delta = 0.0;
  float avg_sched_delta = 0.0;
  int64_t min_sched_delta_ts = 0;
  int64_t max_sched_delta_ts = 0;
  int64_t data_ts = 0;
  float min_ipc = 0.0;
  float max_ipc = 0.0;
  float avg_ipc = 0.0;
  int64_t min_ipc_ts = 0;
  int64_t max_ipc_ts = 0;
  friend std::ostream& operator<<(std::ostream& os,
                                  const OutputDataNodeSubInfo& node_sub_info) {
    os << "\n";
    os << "node: " << node_sub_info.node << "\n";
    os << "topic: " << node_sub_info.topic << "\n";
    os << "hz: " << node_sub_info.hz << "\n";
    os << "min_delta: " << node_sub_info.min_delta << "ms\n";
    os << "max_delta: " << node_sub_info.max_delta << "ms\n";
    os << "avg_delta: " << node_sub_info.avg_delta << "ms\n";
    os << "min_proc_delta: " << node_sub_info.min_proc_delta << "ms\n";
    os << "max_proc_delta: " << node_sub_info.max_proc_delta << "ms\n";
    os << "avg_proc_delta: " << node_sub_info.avg_proc_delta << "ms\n";
    os << "min_proc_delta_ts: " << node_sub_info.min_proc_delta_ts << "\n";
    os << "max_proc_delta_ts: " << node_sub_info.max_proc_delta_ts << "\n";
    os << "min_sched_delta: " << node_sub_info.min_sched_delta << "ms\n";
    os << "max_sched_delta: " << node_sub_info.max_sched_delta << "ms\n";
    os << "avg_sched_delta: " << node_sub_info.avg_sched_delta << "ms\n";
    os << "min_sched_delta_ts: " << node_sub_info.min_sched_delta_ts << "\n";
    os << "max_sched_delta_ts: " << node_sub_info.max_sched_delta_ts << "\n";
    os << "min_ipc: " << node_sub_info.min_ipc << "us\n";
    os << "max_ipc: " << node_sub_info.max_ipc << "us\n";
    os << "avg_ipc: " << node_sub_info.avg_ipc << "us\n";
    os << "min_ipc_ts: " << node_sub_info.min_ipc_ts << "\n";
    os << "max_ipc_ts: " << node_sub_info.max_ipc_ts << "\n";
    os << "\n";
    return os;
  }
};

struct OneCpuInfo {
  std::string cpu_name = "";
  float cpu_percent = 0.0;
  float cpu_user_percent = 0.0;
  float cpu_sys_percent = 0.0;
  float cpu_idle_percent = 0.0;
  float cpu_wa_percent = 0.0;
  float cpu_si_percent = 0.0;
  float cpu_hi_percent = 0.0;
  float cpu_ni_percent = 0.0;
  friend std::ostream& operator<<(std::ostream& os, const OneCpuInfo& cpu) {
    os << "\n";
    os << "cpu_name: " << cpu.cpu_name << "\n";
    os << "cpu_percent: " << cpu.cpu_percent << "%\n";
    os << "user_percent: " << cpu.cpu_user_percent << "%\n";
    os << "sys_percent: " << cpu.cpu_sys_percent << "%\n";
    os << "idle_percent: " << cpu.cpu_idle_percent << "%\n";
    os << "wait_percent: " << cpu.cpu_wa_percent << "%\n";
    os << "ni_percent: " << cpu.cpu_ni_percent << "%\n";
    os << "si_percent: " << cpu.cpu_si_percent << "%\n";
    os << "hi_percent: " << cpu.cpu_hi_percent << "%\n";
    os << "\n";
    return os;
  }
};

struct OutputFileSystemStat {
  std::string type = "";
  float total = 0.0;
  float used = 0.0;
  float free = 0.0;
  float used_percent = 0.0;
  std::string mount_point = "";
  float read_req_speed = 0.0;
  float write_req_speed = 0.0;
  float read_kb_speed = 0.0;
  float write_kb_speed = 0.0;
  float read_wait = 0.0;
  float write_wait = 0.0;
  float aqu_sz = 0.0;
  float util = 0.0;  // 磁盘使用率

  friend std::ostream& operator<<(std::ostream& os, const OutputFileSystemStat& fs) {
    os << "\n";
    os << "type: " << fs.type << "\n";
    os << "total: " << fs.total << "GB\n";
    os << "used: " << fs.used << "GB\n";
    os << "free: " << fs.free << "GB\n";
    os << "used_percent: " << fs.used_percent << "%\n";
    os << "mount_point: " << fs.mount_point << "\n";
    os << "read_req_speed: " << fs.read_req_speed << "req/s\n";
    os << "write_req_speed: " << fs.write_req_speed << "req/s\n";
    os << "read_kb_speed: " << fs.read_kb_speed << "KB/s\n";
    os << "write_kb_speed: " << fs.write_kb_speed << "KB/s\n";
    os << "read_wait: " << fs.read_wait << "s\n";
    os << "write_wait: " << fs.write_wait << "s\n";
    os << "aqu_sz: " << fs.aqu_sz << "s\n";
    os << "util: " << fs.util << "%\n";
    os << "\n";
    return os;
  }
};

struct OutputSoftIrq {
  std::string cpu = "";
  float hi = 0.0;
  float timer = 0.0;
  float net_tx = 0.0;
  float net_rx = 0.0;
  float block = 0.0;
  float irq_poll = 0.0;
  float tasklet = 0.0;
  float sched = 0.0;
  float hrtimer = 0.0;
  float rcu = 0.0;

  friend std::ostream& operator<<(std::ostream& os, const OutputSoftIrq& si) {
    os << "\n";
    os << "cpu: " << si.cpu << "\n";
    os << "hi: " << si.hi << "\n";
    os << "timer: " << si.timer << "\n";
    os << "net_tx: " << si.net_tx << "\n";
    os << "net_rx: " << si.net_rx << "\n";
    os << "block: " << si.block << "\n";
    os << "irq_poll: " << si.irq_poll << "\n";
    os << "tasklet: " << si.tasklet << "\n";
    os << "sched: " << si.sched << "\n";
    os << "hrtimer: " << si.hrtimer << "\n";
    os << "rcu: " << si.rcu << "\n";
    os << "\n";
    return os;
  }
};

struct OutputCpuIrq {
  std::string cpu = "";
  std::string irq = "";
  float speed = 0.0;

  friend std::ostream& operator<<(std::ostream& os, const OutputCpuIrq& ci) {
    os << "\n";
    os << "cpu: " << ci.cpu << "\n";
    os << "irq: " << ci.irq << "\n";
    os << "speed: " << ci.speed << "\n";
    os << "\n";
    return os;
  }
};

struct OutputNodeInfo {
  std::string name = "";
  int32_t pid = 0;
  int32_t threads = 0;
  std::string status = "";
  float cpu_used_percent = 0.0;
  float mem_used_percent = 0.0;
  float write_mbytes = 0.0;
  float read_mbytes = 0.0;
  float read_kb_speed = 0.0;
  float write_kb_speed = 0.0;
  std::string sched_policy = "";
  int32_t sched_prio = 0;
  float io_delay = 0.0;
  int32_t voluntary_ctxt_switches = 0;
  int32_t nonvoluntary_ctxt_switches = 0;
  int32_t minflt = 0;
  int32_t majflt = 0;
  float sched_run_time = 0.0;
  float sched_wait_time = 0.0;
  float sched_run_cnts = 0.0;
  float rss_shmem = 0.0;
  float vm_swap = 0.0;
  float cpu_user_percent = 0.0;
  float cpu_sys_percent = 0.0;
  float cpu_wait_percent = 0.0;
  int32_t processor = 0;

  friend std::ostream& operator<<(std::ostream& os, const OutputNodeInfo& ni) {
    os << "\n";
    os << "name: " << ni.name << "\n";
    os << "pid: " << ni.pid << "\n";
    os << "threads: " << ni.threads << "\n";
    os << "status: " << ni.status << "\n";
    os << "cpu_used_percent: " << ni.cpu_used_percent << "%\n";
    os << "mem_used_percent: " << ni.mem_used_percent << "%\n";
    os << "write_mbytes: " << ni.write_mbytes << "MB\n";
    os << "read_mbytes: " << ni.read_mbytes << "MB\n";
    os << "read_kb_speed: " << ni.read_kb_speed << "KB/s\n";
    os << "write_kb_speed: " << ni.write_kb_speed << "KB/s\n";
    os << "sched_policy: " << ni.sched_policy << "\n";
    os << "sched_prio: " << ni.sched_prio << "\n";
    os << "io_delay: " << ni.io_delay << "s\n";
    os << "voluntary_ctxt_switches: " << ni.voluntary_ctxt_switches << "\n";
    os << "nonvoluntary_ctxt_switches: " << ni.nonvoluntary_ctxt_switches
       << "\n";
    os << "minflt: " << ni.minflt << "\n";
    os << "majflt: " << ni.majflt << "\n";
    os << "sched_run_time: " << ni.sched_run_time << "s\n";
    os << "sched_wait_time: " << ni.sched_wait_time << "s\n";
    os << "sched_run_cnts: " << ni.sched_run_cnts << "\n";
    os << "rss_shmem: " << ni.rss_shmem << "MB\n";
    os << "vm_swap: " << ni.vm_swap << "MB\n";
    os << "cpu_user_percent: " << ni.cpu_user_percent << "%\n";
    os << "cpu_sys_percent: " << ni.cpu_sys_percent << "%\n";
    os << "cpu_wait_percent: " << ni.cpu_wait_percent << "%\n";
    os << "processor: " << ni.processor << "\n";
    os << "\n";
    return os;
  }
};

struct OutputNetInfo {
  std::string name = "";
  std::string status = "";
  int32_t mtu = 0;
  float send_rate = 0.0;
  float rcv_rate = 0.0;
  float send_pkts_rate = 0.0;
  float rcv_pkts_rate = 0.0;
  friend std::ostream& operator<<(std::ostream& os, const OutputNetInfo& net) {
    os << "\n";
    os << "name: " << net.name << "\n";
    os << "status: " << net.status << "\n";
    os << "mtu: " << net.mtu << "\n";
    os << "send_rate: " << net.send_rate << "KB/s\n";
    os << "rcv_rate: " << net.rcv_rate << "KB/s\n";
    os << "send_pkts_rate: " << net.send_pkts_rate << "pkt/s\n";
    os << "rcv_pkts_rate: " << net.rcv_pkts_rate << "pkt/s\n";
    os << "\n";
    return os;
  }
};

struct OutputTcpInfo {
  float in_speed = 0.0;
  float out_speed = 0.0;
  float retrans_speed = 0.0;
  int64_t in_errs = 0;
  int64_t out_rsts = 0;

  friend std::ostream& operator<<(std::ostream& os, const OutputTcpInfo& tcp) {
    os << "\n";
    os << "in_speed: " << tcp.in_speed << "KB/s\n";
    os << "out_speed: " << tcp.out_speed << "KB/s\n";
    os << "retrans_speed: " << tcp.retrans_speed << "KB/s\n";
    os << "in_errs: " << tcp.in_errs << "\n";
    os << "out_rsts: " << tcp.out_rsts << "\n";
    os << "\n";
    return os;
  }
};

struct OutputUdpInfo {
  float in_speed = 0.0;
  float out_speed = 0.0;
  int64_t in_errs = 0;
  int64_t rcvbuf_errs = 0;
  int64_t sndbuf_errs = 0;

  friend std::ostream& operator<<(std::ostream& os, const OutputUdpInfo& udp) {
    os << "\n";
    os << "in_speed: " << udp.in_speed << "KB/s\n";
    os << "out_speed: " << udp.out_speed << "KB/s\n";
    os << "in_errs: " << udp.in_errs << "\n";
    os << "rcvbuf_errs: " << udp.rcvbuf_errs << "\n";
    os << "sndbuf_errs: " << udp.sndbuf_errs << "\n";
    os << "\n";
    return os;
  }
};

struct OutputIpInfo {
  float in_hdr_errs_speed = 0.0;
  float in_addr_errs_speed = 0.0;
  float in_unkown_protos_speed = 0.0;
  float in_discards_speed = 0.0;
  float out_discards_speed = 0.0;
  float out_no_routes_speed = 0.0;
  float reasm_timeout_speed = 0.0;
  float reasm_reqds_speed = 0.0;
  float reasm_oks_speed = 0.0;
  float reasm_fails_speed = 0.0;
  float frag_oks_speed = 0.0;
  float frag_fails_speed = 0.0;
  float frag_creates_speed = 0.0;

  friend std::ostream& operator<<(std::ostream& os, const OutputIpInfo& ip) {
    os << "\n";
    os << "in_hdr_errs_speed: " << ip.in_hdr_errs_speed << "/s\n";
    os << "in_addr_errs_speed: " << ip.in_addr_errs_speed << "/s\n";
    os << "in_unkown_protos_speed: " << ip.in_unkown_protos_speed << "/s\n";
    os << "in_discards_speed: " << ip.in_discards_speed << "/s\n";
    os << "out_discards_speed: " << ip.out_discards_speed << "/s\n";
    os << "out_no_routes_speed: " << ip.out_no_routes_speed << "/s\n";
    os << "reasm_timeout_speed: " << ip.reasm_timeout_speed << "/s\n";
    os << "reasm_oks_speed: " << ip.reasm_oks_speed << "/s\n";
    os << "reasm_fails_speed: " << ip.reasm_fails_speed << "/s\n";
    os << "frag_oks_speed: " << ip.frag_oks_speed << "/s\n";
    os << "frag_fails_speed: " << ip.frag_fails_speed << "/s\n";
    os << "frag_creates_speed: " << ip.frag_creates_speed << "/s\n";
    os << "\n";
    return os;
  }
};

struct OutputCpuTemp {
  std::string description = "";
  float temperature = 0.0;
  float high = 0.0;
  float crit = 0.0;

  friend std::ostream& operator<<(std::ostream& os,
                                  const OutputCpuTemp& cpu_temp) {
    os << "\n";
    os << "description: " << cpu_temp.description << "\n";
    os << "temperature: " << cpu_temp.temperature << "°C\n";
    os << "high: " << cpu_temp.high << "°C\n";
    os << "crit: " << cpu_temp.crit << "°C\n";
    os << "\n";
    return os;
  }
};

struct OutputGpuProcess {
  std::string name = "";
  int32_t pid = 0;
  int32_t mem_used = 0;
  int32_t gpu_used = 0;
  friend std::ostream& operator<<(std::ostream& os, const OutputGpuProcess& gpu_process) {
    os << "\n";
    os << "name: " << gpu_process.name << "\n";
    os << "pid: " << gpu_process.pid << "\n";
    os << "mem_used: " << gpu_process.mem_used << "MB\n";
    os << "gpu_used: " << gpu_process.gpu_used << "%\n";
    os << "\n";
    return os;
  }
};

struct OutputData {
  std::string topic_name = "/system_stats";
  OutputDataCpu cpu;
  OutputCpuLoad cpu_load;
  OutputDataMem mem;
  OutputDataMemDetail mem_detail;
  std::vector<OutputDataNodePubInfo> node_pub_infos;
  std::vector<OutputDataNodeSubInfo> node_sub_infos;
  std::vector<OneCpuInfo> cpu_infos;
  std::vector<OutputFileSystemStat> filesystem_infos;
  std::vector<OutputSoftIrq> softirq_infos;
  std::vector<OutputCpuIrq> cpu_irq_infos;
  std::vector<OutputNodeInfo> node_infos;
  std::vector<OutputNetInfo> net_infos;
  OutputTcpInfo tcp_info;
  OutputUdpInfo udp_info;
  OutputIpInfo ip_info;
  std::vector<OutputCpuTemp> cpu_temp_infos;
  std::vector<OutputGpuInfo> gpu_infos;
  std::vector<OutputGpuProcess> gpu_processes;
  public:
  OutputNodeInfo& AddNodeInfo() {
    OutputNodeInfo& new_node_info = node_infos.emplace_back();
    return new_node_info;
  }
  OutputGpuInfo& AddGpuInfo() {
    OutputGpuInfo& new_gpu_info = gpu_infos.emplace_back();
    return new_gpu_info;
  }

  OutputGpuProcess& AddGpuProcess() {
    OutputGpuProcess& new_gpu_process = gpu_processes.emplace_back();
    return new_gpu_process;
  }
  void clear() {
    cpu = {};
    cpu_load = {};
    mem = {};
    mem_detail = {};
    node_pub_infos.clear();
    node_sub_infos.clear();
    cpu_infos.clear();
    filesystem_infos.clear();
    softirq_infos.clear();
    cpu_irq_infos.clear();
    node_infos.clear();
    net_infos.clear();
    tcp_info = {};
    udp_info = {};
    ip_info = {};
    cpu_temp_infos.clear();
    gpu_infos.clear();
    gpu_processes.clear();
  }
};

}  // namespace system_stats
}  // namespace mw