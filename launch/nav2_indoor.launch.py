#!/usr/bin/env python3

import os
from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, IncludeLaunchDescription
from launch.substitutions import LaunchConfiguration
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch_ros.actions import Node


def generate_launch_description():
    # 获取配置文件路径
    config_dir = os.path.join(os.path.dirname(__file__), '..', 'config')
    nav2_params_file = os.path.join(config_dir, 'nav2_params_indoor.yaml')
    amcl_params_file = os.path.join(config_dir, 'amcl.yaml')

    # 声明启动参数
    declare_map_yaml_cmd = DeclareLaunchArgument(
        'map',
        description='Full path to map yaml file to load')

    declare_use_sim_time_cmd = DeclareLaunchArgument(
        'use_sim_time',
        default_value='false',
        description='Use simulation (Gazebo) clock if true')

    # 地图文件路径
    map_yaml_file = LaunchConfiguration('map')
    use_sim_time = LaunchConfiguration('use_sim_time')

    # Map server
    map_server_cmd = Node(
        package='nav2_map_server',
        executable='map_server',
        name='map_server',
        output='screen',
        parameters=[{'use_sim_time': use_sim_time},
                    {'yaml_filename': map_yaml_file}])

    # AMCL
    amcl_cmd = Node(
        package='nav2_amcl',
        executable='amcl',
        name='amcl',
        output='screen',
        parameters=[amcl_params_file,
                    {'use_sim_time': use_sim_time}])

    # Lifecycle manager for map_server and amcl
    lifecycle_manager_localization_cmd = Node(
        package='nav2_lifecycle_manager',
        executable='lifecycle_manager',
        name='lifecycle_manager_localization',
        output='screen',
        parameters=[{'use_sim_time': use_sim_time},
                    {'autostart': True},
                    {'node_names': ['map_server', 'amcl']}])

    # Nav2 controller server
    controller_server_cmd = Node(
        package='nav2_controller',
        executable='controller_server',
        output='screen',
        parameters=[nav2_params_file,
                    {'use_sim_time': use_sim_time}])

    # Nav2 planner server
    planner_server_cmd = Node(
        package='nav2_planner',
        executable='planner_server',
        name='planner_server',
        output='screen',
        parameters=[nav2_params_file,
                    {'use_sim_time': use_sim_time}])

    # Nav2 behavior server
    behavior_server_cmd = Node(
        package='nav2_behaviors',
        executable='behavior_server',
        name='behavior_server',
        output='screen',
        parameters=[nav2_params_file,
                    {'use_sim_time': use_sim_time}])

    # Nav2 BT navigator
    bt_navigator_cmd = Node(
        package='nav2_bt_navigator',
        executable='bt_navigator',
        name='bt_navigator',
        output='screen',
        parameters=[nav2_params_file,
                    {'use_sim_time': use_sim_time}])

    # Nav2 waypoint follower
    waypoint_follower_cmd = Node(
        package='nav2_waypoint_follower',
        executable='waypoint_follower',
        name='waypoint_follower',
        output='screen',
        parameters=[nav2_params_file,
                    {'use_sim_time': use_sim_time}])

    # Nav2 velocity smoother
    velocity_smoother_cmd = Node(
        package='nav2_velocity_smoother',
        executable='velocity_smoother',
        name='velocity_smoother',
        output='screen',
        parameters=[nav2_params_file,
                    {'use_sim_time': use_sim_time}])

    # Lifecycle manager for navigation
    lifecycle_manager_navigation_cmd = Node(
        package='nav2_lifecycle_manager',
        executable='lifecycle_manager',
        name='lifecycle_manager_navigation',
        output='screen',
        parameters=[{'use_sim_time': use_sim_time},
                    {'autostart': True},
                    {'node_names': ['controller_server',
                                    'planner_server',
                                    'behavior_server',
                                    'bt_navigator',
                                    'waypoint_follower',
                                    'velocity_smoother']}])

    # Create the launch description and populate
    ld = LaunchDescription()

    # Declare the launch options
    ld.add_action(declare_map_yaml_cmd)
    ld.add_action(declare_use_sim_time_cmd)

    # Add the nodes
    ld.add_action(map_server_cmd)
    ld.add_action(amcl_cmd)
    ld.add_action(lifecycle_manager_localization_cmd)
    ld.add_action(controller_server_cmd)
    ld.add_action(planner_server_cmd)
    ld.add_action(behavior_server_cmd)
    ld.add_action(bt_navigator_cmd)
    ld.add_action(waypoint_follower_cmd)
    ld.add_action(velocity_smoother_cmd)
    ld.add_action(lifecycle_manager_navigation_cmd)

    return ld
