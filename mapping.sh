#!/bin/bash

# 室内建图脚本
# 用于创建室内地图，使用 slam_toolbox 进行 2D SLAM 建图

# 获取脚本所在目录的绝对路径
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# 清理 Fast-DDS 共享内存文件，避免 RTPS_TRANSPORT_SHM 错误
echo "=========================================="
echo "清理 Fast-DDS 共享内存..."
echo "=========================================="
sudo rm -f /dev/shm/fastrtps_* 2>/dev/null || true
sudo rm -f /dev/shm/fast_datasharing_* 2>/dev/null || true
echo "清理完成"
echo ""

echo "=========================================="
echo "室内建图系统启动"
echo "=========================================="
echo ""
echo "启动节点："
echo "  - dog_control (机器狗控制)"
echo "  - livox_mid360 (Mid360 雷达驱动)"
echo "  - fast_lio (FAST-LIO2 定位)"
echo "  - pointcloud_to_laserscan (3D→2D 转换)"
echo "  - slam_toolbox (2D SLAM 建图)"
echo ""
echo "建图流程："
echo "  1. 等待所有节点启动完成"
echo "  2. 在另一个终端启动键盘控制："
echo "     ros2 run d1_dog_control keyboard_teleop_node"
echo "  3. 使用键盘控制机器狗走一遍环境 (w/s/a/d/q/e)"
echo "  4. 在另一个终端保存地图："
echo "     ./save_map.sh maps/地图名"
echo "  5. 按 Ctrl+C 退出"
echo ""
echo "=========================================="
echo ""

# 清理函数
cleanup() {
    echo ""
    echo "=========================================="
    echo "正在关闭建图系统..."
    echo "=========================================="

    # 让机器狗趴下
    echo "让机器狗趴下..."
    ros2 topic pub --once /robot_cmd std_msgs/msg/String "{data: 'sit'}"
    sleep 2

    # 终止所有后台进程
    echo "终止所有节点..."
    kill $(jobs -p) 2>/dev/null
    wait

    echo "建图系统已关闭"
    echo ""
    exit 0
}

# 捕获 Ctrl+C 信号
trap cleanup SIGINT SIGTERM

# 1. 启动机器狗控制节点
echo "启动机器狗控制节点..."
ros2 run d1_dog_control dog_control_node &
sleep 2

# 让机器狗站立
echo "让机器狗站立..."
ros2 topic pub --once /robot_cmd std_msgs/msg/String "{data: 'stand'}"
sleep 3

# 2. 启动 Mid360 雷达驱动
echo "启动 Mid360 雷达驱动..."
ros2 launch "$SCRIPT_DIR/launch/livox_mid360.launch.py" &
sleep 3

# 3. 启动 FAST-LIO2
echo "启动 FAST-LIO2..."
ros2 launch "$SCRIPT_DIR/launch/fast_lio.launch.py" &
sleep 3

# 4. 启动 pointcloud_to_laserscan
echo "启动 pointcloud_to_laserscan..."
ros2 launch "$SCRIPT_DIR/launch/pointcloud_to_laserscan.launch.py" &
sleep 2

# 5. 启动 slam_toolbox
echo "启动 slam_toolbox..."
ros2 launch "$SCRIPT_DIR/launch/slam_toolbox.launch.py" &
sleep 2

echo ""
echo "=========================================="
echo "所有节点已启动"
echo "=========================================="
echo ""
echo "下一步操作："
echo "  1. 启动 RViz2 查看建图过程："
echo "     rviz2 -d rviz/mapping.rviz"
echo ""
echo "  2. 在另一个终端启动键盘控制："
echo "     ros2 run d1_dog_control keyboard_teleop_node"
echo ""
echo "  3. 使用键盘控制机器狗走一遍环境："
echo "     w - 前进"
echo "     s - 后退"
echo "     a - 左移"
echo "     d - 右移"
echo "     q - 左转"
echo "     e - 右转"
echo "     x - 停止"
echo ""
echo "  4. 建图完成后，在另一个终端保存地图："
echo "     ./save_map.sh maps/地图名"
echo ""
echo "  5. 按 Ctrl+C 退出建图系统"
echo ""
echo "=========================================="
echo ""

# 等待用户按 Ctrl+C
wait
