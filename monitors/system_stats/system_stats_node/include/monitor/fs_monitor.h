#pragma once
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <sys/types.h>
#include <sys/vfs.h>
#include <unistd.h>

#include <boost/chrono.hpp>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "monitor/monitor_item.h"

namespace mw {
namespace system_stats {

class FsMonitor : public MonitorItem {
  static const int kSectorBytes = 512;
  struct DiskStat {
    dev_t dev_no;
    std::string name;
    std::string mount_point;
    int64_t read_reqs;
    int64_t read_merged;
    int64_t read_sectors;
    int64_t read_spent;
    int64_t write_reqs;
    int64_t write_merged;
    int64_t write_sectors;
    int64_t write_spent;
    int64_t tot_ticks;
    int64_t req_ticks;
    int64_t timestamp;
    uint32_t minor;
    uint32_t major;
  };

 public:
  explicit FsMonitor(mw::proto::SystemStatsConfig &cfg) : MonitorItem(cfg) {}
  int32_t Start() override;
  int32_t RunOnce(OutputData &msg) override;
  int32_t Stop() override { return 0; }
  const std::string Name() const override { return "FsMonitor"; }

 private:
  boost::chrono::steady_clock::time_point last_time_;
  std::map<std::string, DiskStat> all_diskstats_;
  std::set<std::string> monitor_mount_points_;
  std::string system_mount_points_;
  void pub_all_disks(const std::map<dev_t, DiskStat> &all_disks,
                     std::set<std::string> &pub_disks, OutputData &msg);

  void pub_one_disk_ext(const DiskStat &cur_disk, const DiskStat &prev_disk,
                        OutputFileSystemStat &one_disk_msg);
  std::string get_partition_name(const std::string &device);
  bool get_all_diskstats(OutputData &msg);
  bool get_all_disks(std::map<dev_t, DiskStat> &all_disks);
  void get_system_partition(OutputData &msg);
};

}  // namespace system_stats
}  // namespace mw