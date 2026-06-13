#include <iostream>

#include "stats_output.h"

namespace mw {
namespace system_stats {

class ConsoleOutput : public StatsOutput {
 public:
  ConsoleOutput() = default;

  ~ConsoleOutput() = default;

  int32_t Output(const OutputData &data) override;
};

}  // namespace system_stats
}  // namespace mw