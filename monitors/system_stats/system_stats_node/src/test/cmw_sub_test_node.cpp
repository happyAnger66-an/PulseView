#include <fcntl.h>
#include <string>
#include <unordered_map>

#include <mw_base/base_dir.h>
#include <mw_base/now.h>
#include <common/status.h>
#include <glog/logging.h>
#include <module_protos/module_config.pb.h>

#include "cmw_sub_test_node.h"

namespace mw {

namespace system_stats {

bool CmwSubNode::Init() {
  std::string topic = "/system_info_orin_0";
  RegisterSub<mw::proto::SystemStats>(
      topic, [this](const mw::proto::SystemStats &msg) {
        std::cout << "msg " << msg.DebugString() << std::endl;
      });

  return true;
}

CmwSubNode::~CmwSubNode() {}

}  // namespace system_stats
}  // namespace mw
