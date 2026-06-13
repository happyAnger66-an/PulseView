set(system_stats_SRCS
  system_stats.cpp
  system_stats_node.cpp
  monitor/node_latency_monitor.cpp
  monitor/cpu_monitor.cpp
  monitor/mem_monitor.cpp
  monitor/fs_monitor.cpp
  monitor/softirq_monitor.cpp
  monitor/node_monitor.cpp
  monitor/net_monitor.cpp
  monitor/cpu_temp_monitor.cpp
  monitor/gpu_monitor.cpp
  monitor/jtop_monitor.cpp
  utils/proc_stat.cpp
  utils/process.cpp
  utils/libsensors_chip.cpp
  utils/jetsonpower.cpp
  outputs/stats_output.cpp
  outputs/stats_console_output.cpp
  outputs/stats_ros2msg_output.cpp
  diag/cpu_diag_item.cpp
  diag/mem_diag_item.cpp
  diag/stats_diag.cpp
  diag/pub_topic_diag_item.cpp
  diag/sub_topic_diag_item.cpp
  diag/fs_diag_item.cpp
)

add_executable(${PROJECT_NAME}
  ${system_stats_SRCS}
)

target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_17)

target_include_directories(${PROJECT_NAME} PUBLIC
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>"
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../include>"
  "$<INSTALL_INTERFACE:include>")

target_link_libraries(${PROJECT_NAME}
  glog::glog
  absl::str_format
  boost_filesystem
  boost_chrono
  ${PROTOBUF_LIBRARIES}
)

# specific order: dependents before dependencies
ament_target_dependencies(${PROJECT_NAME}
  mw_base
  mw_shm
  mw_proto_util
  mw_common
  rclcpp
  system_stats_protos
  system_stats_interfaces
  diagnostic_msgs
  mw_startup
)

# Export old-style CMake variables
ament_export_include_directories(include)
ament_export_libraries(${PROJECT_NAME})

# Export modern CMake targets
ament_export_targets(${PROJECT_NAME})
ament_export_dependencies(mw_base)
ament_export_dependencies(mw_shm)
ament_export_dependencies(mw_common)
ament_export_dependencies(mw_proto_util)
ament_export_dependencies(rclcpp)
ament_export_dependencies(boost_filesystem)
ament_export_dependencies(diagnostic_msgs)

if(BUILD_TESTS)
add_subdirectory(test)
endif(BUILD_TESTS)

install(TARGETS
${PROJECT_NAME}
  DESTINATION lib/${PROJECT_NAME}
)

#------------- Install -------------#
install(
  TARGETS ${PROJECT_NAME} EXPORT ${PROJECT_NAME}
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
)