#!/bin/bash

# Agibot D1 导航系统编译脚本
# 使用方法: ./build.sh

set -e  # 遇到错误立即退出

echo "=========================================="
echo "  Agibot D1 导航系统 - 编译脚本"
echo "=========================================="
echo ""

# 获取脚本所在目录
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd "$SCRIPT_DIR"

echo "工作空间路径: $SCRIPT_DIR"
echo ""

# 检查是否在 ROS2 环境中
if [ -z "$ROS_DISTRO" ]; then
    echo "错误: ROS2 环境未加载"
    echo "请先执行: source /opt/ros/humble/setup.bash"
    exit 1
fi

echo "ROS2 发行版: $ROS_DISTRO"
echo ""

# 清理旧的编译文件（可选）
read -p "是否清理旧的编译文件? (y/N): " -n 1 -r
echo
if [[ $REPLY =~ ^[Yy]$ ]]; then
    echo "清理 build, install, log 目录..."
    rm -rf build install log
    echo "清理完成"
    echo ""
fi

# 编译工作空间
echo "开始编译..."
echo ""

colcon build --symlink-install

# 检查编译结果
if [ $? -eq 0 ]; then
    echo ""
    echo "=========================================="
    echo "  编译成功！"
    echo "=========================================="
    echo ""
    echo "下一步:"
    echo "  1. 加载环境: source install/setup.bash"
    echo "  2. 运行系统: ./run.sh"
    echo ""
else
    echo ""
    echo "=========================================="
    echo "  编译失败！"
    echo "=========================================="
    echo ""
    exit 1
fi
