#include "mw_monitor/topics_graph_cfg.h"

template <>
std::once_flag
    mw_comm::Singleton<mw::system_stats::TopicsGraphCfg>::flag{};

template <>
std::unique_ptr<mw::system_stats::TopicsGraphCfg>
    mw_comm::Singleton<mw::system_stats::TopicsGraphCfg>::instance_ =
        nullptr;
