#include "cmw_sub_test_node.h"

#include <mw_base/base_dir.h>
#include <mw_base/now.h>
#include <glog/logging.h>

#include "log/async_logsink_entry.h"

int main(int argc, char **argv) {
  google::ParseCommandLineFlags(&argc, &argv, true);
  google::InitGoogleLogging(argv[0]);
  google::InstallFailureSignalHandler();
  mw::common::logsink::InitAsyncLogSink(argv[0]);

  std::string cfg_file = mw_base::GetBaseDirPath(mw_base::BaseDir::kConfigDir) +
                         "/config/node/cmw_sub_test/sub_test.pb.conf";

  mw::system_stats::CmwSubNode node(cfg_file);
  node.Init();
  node.Start();

  //  ros::spin();
  mw::WaitForShutdown();

  return 0;
}
