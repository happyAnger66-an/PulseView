cmake_minimum_required(VERSION 3.8)
project(system_stats_node)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

set(CMAKE_BUILD_TYPE Debug)
add_compile_options(-g)
# find dependencies
find_package(ament_cmake REQUIRED)
find_package(mw_base REQUIRED)
find_package(mw_shm REQUIRED)
find_package(mw_proto_util REQUIRED)
find_package(mw_common REQUIRED)
find_package(system_stats_protos REQUIRED)
find_package(system_stats_interfaces REQUIRED)
find_package(rclcpp REQUIRED)
find_package(glog REQUIRED)
find_package(absl REQUIRED)
find_package(Boost REQUIRED COMPONENTS filesystem chrono)
find_package(Protobuf REQUIRED)
find_package(diagnostic_msgs REQUIRED)
find_package(mw_startup REQUIRED)
# uncomment the following section in order to fill in
# further dependencies manually.
# find_package(<dependency> REQUIRED)

if (CMAKE_SYSTEM_PROCESSOR MATCHES "(arm64)|(aarch64)")
link_libraries(-Wl,/usr/lib/aarch64-linux-gnu/libsensors.so.5)
add_subdirectory(src)
else()
link_libraries(-Wl,/usr/lib/x86_64-linux-gnu/libsensors.so.5)
add_subdirectory(src)
endif()

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  # the following line skips the linter which checks for copyrights
  # comment the line when a copyright and license is added to all source files
  set(ament_cmake_copyright_FOUND TRUE)
  # the following line skips cpplint (only works in a git repo)
  # comment the line when this package is in a git repo and when
  # a copyright and license is added to all source files
  set(ament_cmake_cpplint_FOUND TRUE)
  ament_lint_auto_find_test_dependencies()
endif()

ament_package()
