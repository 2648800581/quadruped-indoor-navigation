cmake_minimum_required(VERSION 3.8)
project(d1_dog_control)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Find dependencies
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(std_msgs REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(nav_msgs REQUIRED)
find_package(tf2_ros REQUIRED)

# Note: This package requires a proprietary robot SDK
# Place SDK files in mc_sdk_lib/ directory before building:
#   - mc_sdk_lib/lib/libmc_sdk_*.so
#   - mc_sdk_lib/include/highlevel.h

# Include directories
include_directories(
  include
  ${CMAKE_CURRENT_SOURCE_DIR}/mc_sdk_lib/include
)

# Link directories
link_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/mc_sdk_lib/lib
)

# Dog control node
add_executable(dog_control_node src/dog_control_node.cpp)
ament_target_dependencies(dog_control_node rclcpp std_msgs geometry_msgs nav_msgs tf2_ros)

# Detect architecture and link appropriate SDK library
if(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64")
  target_link_libraries(dog_control_node mc_sdk_aarch64 pthread)
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64")
  target_link_libraries(dog_control_node mc_sdk_x86_64 pthread)
else()
  message(FATAL_ERROR "Unsupported architecture: ${CMAKE_SYSTEM_PROCESSOR}")
endif()

# Keyboard teleop node
add_executable(keyboard_teleop_node src/keyboard_teleop_node.cpp)
ament_target_dependencies(keyboard_teleop_node rclcpp std_msgs geometry_msgs)

# Install targets
install(TARGETS
  dog_control_node
  keyboard_teleop_node
  DESTINATION lib/${PROJECT_NAME}
)

# Install launch files
install(DIRECTORY launch
  DESTINATION share/${PROJECT_NAME}
)

ament_package()
