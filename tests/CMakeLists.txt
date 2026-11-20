find_package(Qt6 REQUIRED COMPONENTS Test)

add_executable(aember-monitor-unittest test_monitor.cpp)
set_target_properties(aember-monitor-unittest PROPERTIES AUTOMOC ON)
target_link_libraries(aember-monitor-unittest PRIVATE aember-monitor-lib Qt6::Test)
target_compile_definitions(
  aember-monitor-unittest
  PRIVATE AEMBER_MONITOR_FONT_DIR="${CMAKE_SOURCE_DIR}/assets/fonts")
add_test(NAME aember-monitor-unittest COMMAND aember-monitor-unittest)
