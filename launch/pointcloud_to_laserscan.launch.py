#!/usr/bin/env python3
"""
3D点云转2D激光扫描启动文件
将FAST-LIO2的3D点云投影到2D平面，用于Nav2导航
"""

import os
from launch import LaunchDescription
from launch_ros.actions import Node
from ament_index_python.packages import get_package_share_directory


def generate_launch_description():
    # 获取配置文件路径
    config_file = os.path.join(
        os.path.dirname(os.path.dirname(os.path.abspath(__file__))),
        'config',
        'pointcloud_to_laserscan.yaml'
    )

    # pointcloud_to_laserscan节点
    pointcloud_to_laserscan_node = Node(
        package='pointcloud_to_laserscan',
        executable='pointcloud_to_laserscan_node',
        name='pointcloud_to_laserscan',
        parameters=[config_file],
        remappings=[
            ('cloud_in', '/cloud_registered'),  # 订阅FAST-LIO2的点云
            ('scan', '/scan')                    # 发布2D激光扫描
        ],
        output='screen'
    )

    return LaunchDescription([
        pointcloud_to_laserscan_node
    ])
