import os
from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch_ros.actions import Node

def generate_launch_description():
    # Get the config file path from our project
    config_path = os.path.join(
        os.path.dirname(os.path.dirname(os.path.abspath(__file__))),
        'config', 'fast_lio_mid360.yaml')

    fast_lio_node = Node(
        package='fast_lio',
        executable='fastlio_mapping',
        name='fastlio_mapping',
        output='screen',
        parameters=[config_path]
    )

    return LaunchDescription([fast_lio_node])
