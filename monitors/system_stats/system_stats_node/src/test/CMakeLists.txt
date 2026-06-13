add_executable(mw_monitor_test
  test_get_all_topics.cpp)

target_link_libraries(mw_monitor_test PUBLIC
  mw::monitor)

install(TARGETS mw_monitor_test
  ARCHIVE DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
  LIBRARY DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
  RUNTIME DESTINATION ${CATKIN_PACKAGE_BIN_DESTINATION}
)

add_executable(cmw_sub_test
  cmw_sub_test_main.cpp
  cmw_sub_test_node.cpp
)

target_link_libraries(cmw_sub_test PUBLIC
  mw::module
  mw::system_stats::proto
  mw::log
)

install(TARGETS cmw_sub_test
  ARCHIVE DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
  LIBRARY DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
  RUNTIME DESTINATION ${CATKIN_PACKAGE_BIN_DESTINATION}
)