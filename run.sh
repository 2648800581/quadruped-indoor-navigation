#!/bin/bash

# Agibot D1 室内导航系统运行脚本
# 使用方法:
#   ./run.sh --map maps/地图名  # 室内导航模式（需要指定地图）

set -e  # 遇到错误立即退出

# 清理 Fast-DDS 共享内存文件，避免 RTPS_TRANSPORT_SHM 错误
echo "=========================================="
echo "清理 Fast-DDS 共享内存..."
echo "=========================================="
sudo rm -f /dev/shm/fastrtps_* 2>/dev/null || true
sudo rm -f /dev/shm/fast_datasharing_* 2>/dev/null || true
echo "清理完成"
echo ""

# 解析命令行参数
MAP_FILE=""

while [[ $# -gt 0 ]]; do
    case $1 in
        --map)
            MAP_FILE="$2"
            shift 2
            ;;
        *)
            echo "错误: 未知参数 '$1'"
            echo "使用方法:"
            echo "  ./run.sh --map maps/地图名  # 室内导航模式（需要指定地图）"
            echo ""
            echo "示例:"
            echo "  ./run.sh --map maps/office1"
            exit 1
            ;;
    esac
done

# 必须指定地图
if [ -z "$MAP_FILE" ]; then
    echo "错误: 必须指定地图文件"
    echo "使用方法: ./run.sh --map maps/地图名"
    echo ""
    echo "可用的地图:"
    if [ -d "maps" ]; then
        ls -1 maps/*.yaml 2>/dev/null | sed 's/\.yaml$//' || echo "  (无地图文件)"
    else
        echo "  (maps 目录不存在)"
    fi
    echo ""
    echo "提示: 请先运行 './mapping.sh' 创建地图"
    exit 1
fi

# 检查地图文件是否存在
if [ ! -f "${MAP_FILE}.yaml" ]; then
    echo "错误: 地图文件不存在: ${MAP_FILE}.yaml"
    echo ""
    echo "可用的地图:"
    if [ -d "maps" ]; then
        ls -1 maps/*.yaml 2>/dev/null | sed 's/\.yaml$//' || echo "  (无地图文件)"
    else
        echo "  (maps 目录不存在)"
    fi
    echo ""
    echo "提示: 请先运行 './mapping.sh' 创建地图"
    exit 1
fi

# 转换为绝对路径
MAP_FILE="$(cd "$(dirname "${MAP_FILE}.yaml")" && pwd)/$(basename "${MAP_FILE}.yaml")"

echo "=========================================="
echo "  Agibot D1 室内导航系统"
echo "  地图: $MAP_FILE"
echo "=========================================="
echo ""

# 获取脚本所在目录
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd "$SCRIPT_DIR"

# 检查是否已编译
if [ ! -d "install" ]; then
    echo "错误: 工作空间未编译"
    echo "请先执行: ./build.sh"
    exit 1
fi

# 加载环境
echo "加载 ROS2 环境..."
source /opt/ros/humble/setup.bash
source install/setup.bash
echo ""

# 设置 ROS_DOMAIN_ID
export ROS_DOMAIN_ID=23
echo "ROS_DOMAIN_ID 设置为: $ROS_DOMAIN_ID"
echo ""

# 清理函数 - 在退出时执行
cleanup() {
    echo ""
    echo "=========================================="
    echo "  正在关闭系统..."
    echo "=========================================="
    echo ""

    # 终止所有后台进程（节点会自动让狗趴下）
    echo "关闭所有节点..."
    kill $(jobs -p) 2>/dev/null || true

    # 等待节点完成趴下动作
    echo "等待机器狗趴下..."
    wait 2>/dev/null || true

    echo ""
    echo "=========================================="
    echo "  系统已关闭"
    echo "=========================================="
    echo ""
}

# 注册清理函数
trap cleanup EXIT INT TERM

echo "=========================================="
echo "  启动机器狗控制节点"
echo "=========================================="
echo ""

# 启动机器狗控制节点（后台运行）
ros2 launch d1_dog_control dog_control.launch.py &
DOG_CONTROL_PID=$!

# 等待节点启动
echo "等待机器狗控制节点启动..."
sleep 3
echo ""

echo "=========================================="
echo "  启动 Livox Mid360 雷达驱动"
echo "=========================================="
echo ""

# 启动雷达驱动（后台运行）
ros2 launch "$SCRIPT_DIR/launch/livox_mid360.launch.py" &
LIVOX_PID=$!

# 等待雷达驱动启动
echo "等待雷达驱动启动..."
sleep 2
echo ""

echo "=========================================="
echo "  启动 FAST-LIO2 定位节点"
echo "=========================================="
echo ""

# 启动 FAST-LIO2（后台运行，延迟2秒启动）
sleep 2
ros2 launch "$SCRIPT_DIR/launch/fast_lio.launch.py" &
FASTLIO_PID=$!

# 等待 FAST-LIO2 启动
echo "等待 FAST-LIO2 启动..."
sleep 2
echo ""

echo "=========================================="
echo "  启动 PointCloud to LaserScan 转换节点"
echo "=========================================="
echo ""

# 启动点云转激光扫描（后台运行）
ros2 launch "$SCRIPT_DIR/launch/pointcloud_to_laserscan.launch.py" &
P2L_PID=$!

# 等待转换节点启动
echo "等待点云转换节点启动..."
sleep 2
echo ""

echo "=========================================="
echo "  启动 AMCL 和 Nav2"
echo "=========================================="
echo ""

# 启动 Nav2（包含 map_server 和 AMCL）
ros2 launch "$SCRIPT_DIR/launch/nav2_indoor.launch.py" map:="$MAP_FILE" &
NAV2_PID=$!

# 等待 Nav2 启动
echo "等待 Nav2 和 AMCL 启动..."
sleep 5
echo ""

# 让狗站立（执行3次）
echo "=========================================="
echo "  让机器狗站立"
echo "=========================================="
echo ""

for i in {1..3}; do
    echo "站立命令 $i/3"
    ros2 topic pub --once /robot_cmd std_msgs/msg/String "data: 'stand'"
    sleep 1
done

echo ""
echo "机器狗已站立"
echo ""

echo "=========================================="
echo "  系统运行中（室内导航模式）"
echo "=========================================="
echo ""
echo "提示:"
echo "  - 按 Ctrl+C 关闭系统（会自动让狗趴下）"
echo "  - 如需手动让狗趴下，在另一个终端执行:"
echo "    ros2 topic pub --once /robot_cmd std_msgs/msg/String \"data: 'sit'\""
echo ""
echo "当前运行的节点:"
echo "  - dog_control_node (机器狗控制)"
echo "  - livox_lidar_publisher (Mid360 雷达驱动)"
echo "  - fastlio_mapping (FAST-LIO2 定位)"
echo "  - pointcloud_to_laserscan_node (点云转激光扫描)"
echo "  - map_server (地图服务器)"
echo "  - amcl (自适应蒙特卡洛定位)"
echo "  - controller_server (Nav2 控制器)"
echo "  - planner_server (Nav2 路径规划)"
echo "  - behavior_server (Nav2 行为服务器)"
echo "  - bt_navigator (Nav2 行为树导航)"
echo ""
echo "导航操作:"
echo "  - 在 RViz2 中使用 '2D Pose Estimate' 设置初始位姿"
echo "  - 然后使用 '2D Goal Pose' 设置导航目标"
echo ""

# 保持脚本运行
wait
