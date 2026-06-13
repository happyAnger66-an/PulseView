#include "utils/jetsonpower.h"

namespace mw {
namespace system_stats {

pthread_once_t JetsonPowerLib::once = PTHREAD_ONCE_INIT;
std::shared_ptr<JetsonPowerLib> JetsonPowerLib::jetsonpower_lib = nullptr;

}  // namespace system_stats
}  // namespace mw
