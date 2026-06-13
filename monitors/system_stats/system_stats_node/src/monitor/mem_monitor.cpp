#include <sys/sysinfo.h>

#include "monitor/mem_monitor.h"
#include "utils/proc_file.h"

namespace mw {
namespace system_stats {

static constexpr int KKBToGB = 1024 * 1024;
static constexpr int KKBToMB = 1024;
int32_t MemMonitor::RunOnce(OutputData &output_data) {
  ProcFile mem_proc_file("/proc/meminfo");

  std::string item, mem_size;
  long long total, free, avail, buffers, cached;
  long long swap_cached = 0, active = 0, in_active = 0, active_anon = 0,
            inactive_anon = 0;
  long long active_file = 0, inactive_file = 0, dirty = 0, writeback = 0,
            anon_pages = 0, mapped = 0;
  long long kReclaimable = 0, sReclaimable = 0, sUnreclaim = 0;

  while (mem_proc_file.ReadOneLine(item, mem_size)) {
    if (item == "MemTotal:") {
      total = std::stoll(mem_size);
    } else if (item == "MemFree:") {
      free = std::stoll(mem_size);
    } else if (item == "MemAvailable:") {
      avail = std::stoll(mem_size);
    } else if (item == "Buffers:") {
      buffers = std::stoll(mem_size);
    } else if (item == "Cached:") {
      cached = std::stoll(mem_size);
    } else if (item == "SwapCached:") {
      swap_cached = std::stoll(mem_size);
    } else if (item == "Active:") {
      active = std::stoll(mem_size);
    } else if (item == "Inactive:") {
      in_active = std::stoll(mem_size);
    } else if (item == "Active(anon):") {
      active_anon = std::stoll(mem_size);
    } else if (item == "Inactive(anon):") {
      inactive_anon = std::stoll(mem_size);
    } else if (item == "Active(file):") {
      active_file = std::stoll(mem_size);
    } else if (item == "Inactive(file):") {
      inactive_file = std::stoll(mem_size);
    } else if (item == "Dirty:") {
      dirty = std::stoll(mem_size);
    } else if (item == "Writeback:") {
      writeback = std::stoll(mem_size);
    } else if (item == "AnonPages:") {
      anon_pages = std::stoll(mem_size);
    } else if (item == "Mapped:") {
      mapped = std::stoll(mem_size);
    } else if (item == "KReclaimable:") {
      kReclaimable = std::stoll(mem_size);
    } else if (item == "SReclaimable:") {
      sReclaimable = std::stoll(mem_size);
    } else if (item == "SUnreclaim:") {
      sUnreclaim = std::stoll(mem_size);
    }
  }

  double usage_percent =
      static_cast<double>(total - avail) / static_cast<double>(total) * 100.0;

  auto &mem_info = output_data.mem;
  mem_info.avail_size = ((avail) / KKBToGB);
  mem_info.used_percent = usage_percent;
  mem_info.free_size = (static_cast<float>(free) / KKBToGB);  // For compatible, we set avail to free. cviz and monitor treat
                 // avail as free.
  mem_info.total_size = (static_cast<float>(total) / KKBToGB);
  mem_info.buffers_size = (static_cast<float>(buffers) / KKBToGB);
  mem_info.cached_size = (static_cast<float>(cached) / KKBToGB);

  auto &mem_detail = output_data.mem_detail;
  mem_detail.swap_cached = (static_cast<float>(swap_cached) / KKBToMB);
  mem_detail.active = (static_cast<float>(active) / KKBToMB);
  mem_detail.inactive = (static_cast<float>(in_active) / KKBToMB);
  mem_detail.active_anon = (static_cast<float>(active_anon) / KKBToMB);
  mem_detail.inactive_anon = (static_cast<float>(inactive_anon) / KKBToMB);
  mem_detail.active_file = (static_cast<float>(active_file) / KKBToMB);
  mem_detail.inactive_file = (static_cast<float>(inactive_file) / KKBToMB);
  mem_detail.dirty = (static_cast<float>(dirty) / KKBToMB);
  mem_detail.writeback = (static_cast<float>(writeback) / KKBToMB);
  mem_detail.anon_pages = (static_cast<float>(anon_pages) / KKBToMB);
  mem_detail.mapped = (static_cast<float>(mapped) / KKBToMB);
  mem_detail.kreclaimable = (static_cast<float>(kReclaimable) / KKBToMB);
  mem_detail.sreclaimable = (static_cast<float>(sReclaimable) / KKBToMB);
  mem_detail.sunreclaim = (static_cast<float>(sUnreclaim) / KKBToMB);

  return 0;
}

}  // namespace system_stats
}  // namespace mw