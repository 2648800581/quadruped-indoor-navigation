#!/usr/bin/env python3

from launch import LaunchDescription
from launch_ros.actions import Node
from ament_index_python.packages import get_package_share_directory
import os

def generate_launch_description():
    # 获取配置文件路径
    config_file = os.path.join(
        os.path.dirname(os.path.dirname(os.path.abspath(__file__))),
        'config',
        'slam_toolbox.yaml'
    )

    # slam_toolbox 节点（异步SLAM模式）
    slam_toolbox_node = Node(
        package='slam_toolbox',
        executable='async_slam_toolbox_node',
        name='slam_toolbox',
        output='screen',
        parameters=[config_file],
        remappings=[
            ('scan', '/scan'),
            ('odom', '/fast_lio/odom')
        ]
    )

    return LaunchDescription([
        slam_toolbox_node
    ])
