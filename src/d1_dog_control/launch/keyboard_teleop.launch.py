from launch import LaunchDescription
from launch_ros.actions import Node

def generate_launch_description():
    return LaunchDescription([
        Node(
            package='d1_dog_control',
            executable='keyboard_teleop_node',
            name='keyboard_teleop_node',
            output='screen',
            parameters=[{
                'linear_speed': 0.5,
                'angular_speed': 0.5,
                'max_linear_speed': 1.0,
                'max_angular_speed': 1.0,
            }],
            prefix='xterm -e',  # Run in separate terminal
        ),
    ])
