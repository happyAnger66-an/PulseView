#pragma once

#include <boost/chrono.hpp>
#include <string>

namespace mw {
namespace system_stats {
class Utils {
 public:
  static double SteadyTimeToS(
      const boost::chrono::steady_clock::time_point &t1,
      const boost::chrono::steady_clock::time_point &t2) {
    boost::chrono::duration<double> sec = t1 - t2;
    return sec.count();
  }
};

namespace utils {
std::string GetFileInTripPath(const std::string &module_name);
std::string trim(std::string_view s);
}  // namespace utils

}  // namespace system_stats
}  // namespace mw
