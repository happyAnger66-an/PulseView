#pragma once

#include <memory>
#include <vector>

#include "mw_comm/topics.h"
#include "module/module.h"
#include "monitor/monitor_item.h"
#include "types/canbus_mode.h"

#include "cmw_clcpp/cmw_clcpp.hpp"

namespace mw {
namespace system_stats {

class CmwSubNode : public Module {
 public:
  explicit CmwSubNode(const std::string &config_file) : Module(config_file){};
  ~CmwSubNode();

  virtual bool Init() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(CmwSubNode);
};

}  // namespace system_stats
}  // namespace mw
