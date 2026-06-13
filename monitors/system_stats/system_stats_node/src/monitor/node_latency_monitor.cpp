#include "monitor/node_latency_monitor.h"

#include <absl/strings/str_split.h>
#include <glog/logging.h>

#include <iomanip>
#include <vector>

namespace mw {
namespace system_stats {

static constexpr int KTimeOutInS = 10;
static constexpr int KUsToMs = 1000;
static constexpr int KUsToS = 1000 * 1000;

using namespace shm_stats;

int32_t NodeLatencyMonitor::Start() {
  shm_cnts_mgr_ = shm_stats::ShmCntsMgr::Instance();
  return 0;
}

int32_t NodeLatencyMonitor::Stop() {
  shm_cnts_mgr_.reset();
  return 0;
}

int32_t NodeLatencyMonitor::add_pub_info(const std::string &node,
                                         const std::string &topic,
                                         const shm_stats::ShmCntRecord &record,
                                         OutputData &msg) {
  OutputDataNodePubInfo pub_info{};
  pub_info.node = node;
  pub_info.topic = topic;
  if (record.avg_delta > 0) {
    pub_info.hz = 1000 / record.avg_delta;
  }
  pub_info.min_delta = record.min_delta / KUsToMs;
  pub_info.max_delta = record.max_delta / KUsToMs;
  pub_info.avg_delta = record.avg_delta;
  pub_info.min_delta_ts = record.min_delta_ts;
  pub_info.max_delta_ts = record.max_delta_ts;

  pub_info.min_proc_delta = record.min_sub_proc_delta / KUsToMs;
  pub_info.max_proc_delta = record.max_sub_proc_delta / KUsToMs;
  pub_info.min_proc_delta_ts = record.min_sub_proc_ts;
  pub_info.max_proc_delta_ts = record.max_sub_proc_ts;
  pub_info.avg_proc_delta = record.avg_sub_proc;

  msg.node_pub_infos.push_back(pub_info);
  return 0;
}

int32_t NodeLatencyMonitor::add_sub_info(const std::string &node,
                                         const std::string &topic,
                                         const shm_stats::ShmCntRecord &record,
                                         OutputData &msg) {
  OutputDataNodeSubInfo sub_info{};
  sub_info.node = node;
  sub_info.topic = topic;
  if (record.avg_delta > 0) {
    sub_info.hz = (1000 / record.avg_delta);
  }
  sub_info.min_delta = record.min_delta / KUsToMs;
  sub_info.max_delta = record.max_delta / KUsToMs;
  sub_info.avg_delta = record.avg_delta;
  sub_info.min_delta_ts = record.min_delta_ts;
  sub_info.max_delta_ts = record.max_delta_ts;

  sub_info.min_proc_delta = record.min_sub_proc_delta / KUsToMs;
  sub_info.max_proc_delta = record.max_sub_proc_delta / KUsToMs;
  sub_info.min_proc_delta_ts = record.min_sub_proc_ts;
  sub_info.max_proc_delta_ts = record.max_sub_proc_ts;
  sub_info.avg_proc_delta = record.avg_sub_proc;

  sub_info.min_sched_delta = record.min_sub_sched_delta / KUsToMs;
  sub_info.max_sched_delta = record.max_sub_sched_delta / KUsToMs;
  sub_info.min_sched_delta_ts = record.min_sub_sched_ts;
  sub_info.max_sched_delta_ts = record.max_sub_sched_ts;
  sub_info.avg_sched_delta = record.avg_sub_sched;

  sub_info.min_ipc = record.min_ipc;
  sub_info.max_ipc = record.max_ipc;
  sub_info.min_ipc_ts = record.min_ipc_ts;
  sub_info.max_ipc_ts = record.max_ipc_ts;
  sub_info.avg_ipc = record.avg_ipc;

  msg.node_sub_infos.push_back(sub_info);
  return 0;
}

int32_t NodeLatencyMonitor::RunOnce(OutputData &msg) {
  ShmCntsMgr::CntsRecordVec vec;
  if (ShmCntsMgr::GetAllCntsRecords(vec) != 0) {
    LOG(INFO) << "No Shm Records found";
    return -1;
  }

  for (auto &shm_name : vec) {
    std::string node;
    std::string topic;
    int type;
    if (shm_cnts_mgr_->GetRecordInfo(shm_name, node, topic, type) != 0) {
      LOG(INFO) << "Record Info: " << shm_name << " failed.";
      continue;
    }

    ShmCntRecord record{0};
    int ret = shm_cnts_mgr_->GetRecord(node, topic, type, record);
    if (ret != 0 || record.delta_times < 1) {
      continue;
    }

    switch (type) {
      case 0:
        add_pub_info(node, topic, record, msg);
        break;
      case 1:
        add_sub_info(node, topic, record, msg);
        break;
      default:
        LOG(INFO) << "unkown record type: " << type;
        break;
    }
  }
  return 0;
}

}  // namespace system_stats
}  // namespace mw
