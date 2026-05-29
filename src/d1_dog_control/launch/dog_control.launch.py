#!/usr/bin/env python3

from launch import LaunchDescription
from launch_ros.actions import Node

def generate_launch_description():
    return LaunchDescription([
        Node(
            package='d1_dog_control',
            executable='dog_control_node',
            name='dog_control_node',
            output='screen',
            parameters=[
                {'max_linear_velocity': 0.5},      # 降低最大线速度（原 1.0 m/s -> 0.5 m/s）
                {'max_angular_velocity': 0.5},     # 降低最大角速度（原 1.0 rad/s -> 0.5 rad/s）
                {'acceleration_limit': 1.5},       # 降低加速度限制（更平缓的加减速）
                {'cmd_vel_timeout': 0.5},          # 命令超时时间
                {'control_frequency': 500.0}       # 控制频率
            ]
        )
    ])
