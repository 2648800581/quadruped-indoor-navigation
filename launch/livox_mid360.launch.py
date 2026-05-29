import os
from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch_ros.actions import Node

def generate_launch_description():
    # Get the config file path from our project
    config_path = os.path.join(
        os.path.dirname(os.path.dirname(os.path.abspath(__file__))),
        'config')
    user_config_path = os.path.join(config_path, 'MID360_config.json')

    livox_ros2_params = [
        {"xfer_format": 1},       # 1 = Livox CustomMsg - required by FAST-LIO2 when lidar_type=1
        {"multi_topic": 0},
        {"data_src": 0},
        {"publish_freq": 10.0},
        {"output_data_type": 0},
        {"frame_id": "body"},     # 使用body坐标系，与机器狗TF树一致
        {"lvx_file_path": ""},
        {"user_config_path": user_config_path},
        {"cmdline_input_bd_code": "livox0000000001"}
    ]

    livox_driver = Node(
        package='livox_ros_driver2',
        executable='livox_ros_driver2_node',
        name='livox_lidar_publisher',
        output='screen',
        parameters=livox_ros2_params,
        remappings=[
            ('livox/lidar', '/livox/lidar'),
            ('livox/imu', '/livox/imu'),
        ]
    )

    return LaunchDescription([livox_driver])
