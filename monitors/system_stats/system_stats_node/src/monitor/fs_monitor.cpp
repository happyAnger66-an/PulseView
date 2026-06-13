#include "monitor/fs_monitor.h"

#include <glog/logging.h>

#include <set>

#include "mw_base/now.h"
#include "outputs/output_datas.h"
#include "utils/proc_file.h"

namespace mw {
namespace system_stats {

constexpr static char kFsMountsFile[] = "/proc/mounts";
constexpr static char kFsDiskstatsFile[] = "/proc/diskstats";
constexpr int kFsKB = 1024;
constexpr int kFsMB = 1024 * kFsKB;
constexpr int kFsGB = 1024 * kFsMB;

int32_t FsMonitor::Start() {
  for (const auto &monitor_mt : cfg_.fs_cfg().mount_point()) {
    monitor_mount_points_.insert(monitor_mt);
  }

  system_mount_points_ = cfg_.fs_cfg().system_mount_point();

  return 0;
}

std::string FsMonitor::get_partition_name(const std::string &device) {
  std::size_t slash_pos = device.rfind('/');
  if (slash_pos == std::string::npos) {
    return device;
  }

  return device.substr(slash_pos + 1);
}

bool FsMonitor::get_all_disks(std::map<dev_t, DiskStat> &all_disks) {
  ProcFile disks_proc(kFsDiskstatsFile);
  std::vector<std::string> one_disk;
  while (disks_proc.ReadOneLine(one_disk)) {
    DiskStat one_disk_stat{};
    uint32_t uint_major = std::stoi(one_disk[0]);
    uint32_t uint_minor = std::stoi(one_disk[1]);
    one_disk_stat.dev_no = makedev(uint_major, uint_minor);
    one_disk_stat.minor = uint_minor;
    one_disk_stat.major = uint_major;
    one_disk_stat.name = one_disk[2];
    one_disk_stat.read_reqs = std::stoll(one_disk[3]);
    one_disk_stat.read_merged = std::stoll(one_disk[4]);
    one_disk_stat.read_sectors = std::stoll(one_disk[5]);
    one_disk_stat.read_spent = std::stoll(one_disk[6]);
    one_disk_stat.write_reqs = std::stoll(one_disk[7]);
    one_disk_stat.write_merged = std::stoll(one_disk[8]);
    one_disk_stat.write_sectors = std::stoll(one_disk[9]);
    one_disk_stat.write_spent = std::stoll(one_disk[10]);
    one_disk_stat.tot_ticks = std::stoll(one_disk[12]);
    one_disk_stat.req_ticks = std::stoll(one_disk[13]);
    one_disk_stat.timestamp = mw_base::NowNs();
    all_disks[one_disk_stat.dev_no] = one_disk_stat;
    one_disk.clear();
  }

  return true;
}

void FsMonitor::pub_all_disks(const std::map<dev_t, DiskStat> &all_disks,
                              std::set<std::string> &pub_disks,
                              OutputData &msg) {
  for (const auto &iter : all_disks) {
    const auto &dstat = iter.second;
    if (pub_disks.find(dstat.name) != pub_disks.end()) {
      continue;
    }

    if (dstat.minor != 0) {  // pub disk only, do not pub partitions.
      continue;
    }

    auto iter1 = all_diskstats_.find(dstat.name);
    if (iter1 != all_diskstats_.end()) {
      OutputFileSystemStat one_disk_msg{};
      one_disk_msg.type = dstat.name;

      DiskStat &old_dstat = iter1->second;
      pub_one_disk_ext(dstat, old_dstat, one_disk_msg);
      pub_disks.insert(dstat.name);

      msg.filesystem_infos.push_back(one_disk_msg);
    }
    all_diskstats_[dstat.name] = dstat;
  }
}

void FsMonitor::pub_one_disk_ext(const DiskStat &cur_disk,
                                 const DiskStat &prev_disk,
                                 OutputFileSystemStat &one_disk_msg) {
  double period = (cur_disk.timestamp - prev_disk.timestamp) / KNsToS;
  if (period > 0) {
    float read_req_speed =
        static_cast<float>((cur_disk.read_reqs - prev_disk.read_reqs) / period);
    float write_req_speed = static_cast<float>(
        (cur_disk.write_reqs - prev_disk.write_reqs) / period);
    float read_kb_speed = static_cast<float>(
        (cur_disk.read_sectors - prev_disk.read_sectors) / 2 / period);
    float write_kb_speed = static_cast<float>(
        (cur_disk.write_sectors - prev_disk.write_sectors) / 2 / period);
    one_disk_msg.read_req_speed = read_req_speed;
    one_disk_msg.write_req_speed = write_req_speed;
    one_disk_msg.read_kb_speed = read_kb_speed;
    one_disk_msg.write_kb_speed = write_kb_speed;
  }

  int read_spent_delta = cur_disk.read_spent - prev_disk.read_spent;
  int read_delta = cur_disk.read_reqs - prev_disk.read_reqs;
  float read_wait = 0.0;
  if (read_delta > 0 && read_spent_delta > 0) {
    read_wait = static_cast<float>(read_spent_delta) / read_delta;
  }
  int write_spent_delta = cur_disk.write_spent - prev_disk.write_spent;
  int write_delta = cur_disk.write_reqs - prev_disk.write_reqs;
  float write_wait = 0.0;
  if (write_delta > 0 && write_spent_delta > 0) {
    write_wait = static_cast<float>(write_spent_delta) / write_delta;
  }
  one_disk_msg.read_wait = read_wait;
  one_disk_msg.write_wait = write_wait;

  if (period > 0) {
    float util = (static_cast<float>(cur_disk.tot_ticks - prev_disk.tot_ticks) /
                  (period * 10));
    float aqu_sz =
        (static_cast<float>(cur_disk.req_ticks - prev_disk.req_ticks) /
         (period * 10) / 100.0);
    one_disk_msg.util = util;
    one_disk_msg.aqu_sz = aqu_sz;
  }
}

bool FsMonitor::get_all_diskstats(OutputData &msg) {
  std::map<dev_t, DiskStat> all_disks;
  if (!get_all_disks(all_disks)) {
    return false;
  }

  std::set<std::string> pub_disks;
  for (auto &mount_point : monitor_mount_points_) {
    struct stat stat_buf;
    int ret = stat(mount_point.c_str(), &stat_buf);
    if (ret != 0) {
      continue;
    }

    auto iter = all_disks.find(stat_buf.st_dev);
    if (iter == all_disks.end()) {
      LOG(ERROR) << "Can't find mnt: " << mount_point << " disk info !!!";
      continue;
    }

    DiskStat &dstat = iter->second;
    struct statfs fs_stat;
    statfs(mount_point.c_str(), &fs_stat);

    OutputFileSystemStat one_disk_msg{};
    one_disk_msg.type = dstat.name;
    float total_space = fs_stat.f_blocks * fs_stat.f_bsize;
    float avail_space = fs_stat.f_bavail * fs_stat.f_bsize;
    float free_space = fs_stat.f_bfree * fs_stat.f_bsize;
    float used_percent =
        static_cast<float>((total_space - avail_space) / total_space * 100.0);
    one_disk_msg.total = total_space / kFsGB;
    one_disk_msg.used = (total_space - free_space) / kFsGB;
    one_disk_msg.free = avail_space / kFsGB;
    one_disk_msg.used_percent = used_percent;
    one_disk_msg.mount_point = mount_point;

    if (pub_disks.find(dstat.name) == pub_disks.end()) {
      pub_disks.insert(dstat.name);
    }

    auto iter1 = all_diskstats_.find(dstat.name);
    if (iter1 != all_diskstats_.end()) {
      DiskStat &old_dstat = iter1->second;
      pub_one_disk_ext(dstat, old_dstat, one_disk_msg);
    }
    msg.filesystem_infos.push_back(one_disk_msg);
    all_diskstats_[dstat.name] = dstat;
  }

  pub_all_disks(all_disks, pub_disks, msg);
  return true;
}

void FsMonitor::get_system_partition(OutputData &msg) {
  struct statfs fs_stat;
  statfs(system_mount_points_.c_str(), &fs_stat);

  OutputFileSystemStat one_disk_msg{};
  one_disk_msg.type = "/";
  float total_space = fs_stat.f_blocks * fs_stat.f_bsize;
  float avail_space = fs_stat.f_bavail * fs_stat.f_bsize;
  float free_space = fs_stat.f_bfree * fs_stat.f_bsize;
  float used_percent =
      static_cast<float>((total_space - avail_space) / total_space * 100.0);
  one_disk_msg.total = total_space / kFsGB;
  one_disk_msg.used = (total_space - free_space) / kFsGB;
  one_disk_msg.free = avail_space / kFsGB;
  one_disk_msg.used_percent = used_percent;
  one_disk_msg.mount_point = "/";
  msg.filesystem_infos.push_back(one_disk_msg);
}

int32_t FsMonitor::RunOnce(OutputData &msg) {
  if (!get_all_diskstats(msg)) {
    return -1;
  }

  get_system_partition(msg);
  return 0;
}

}  // namespace system_stats
}  // namespace mw
