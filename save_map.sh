#!/bin/bash

# 地图保存工具
# 用于保存 slam_toolbox 建立的地图

# 检查参数
if [ $# -ne 1 ]; then
    echo "用法: $0 <地图路径/名称>"
    echo ""
    echo "示例:"
    echo "  $0 maps/office"
    echo "  $0 maps/floor1"
    echo ""
    echo "说明:"
    echo "  - 地图将保存为 <名称>.pgm 和 <名称>.yaml"
    echo "  - 如果目录不存在，会自动创建"
    exit 1
fi

MAP_PATH="$1"

# 获取脚本所在目录的绝对路径
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# 提取目录和文件名
MAP_DIR=$(dirname "$MAP_PATH")
MAP_NAME=$(basename "$MAP_PATH")

# 如果是相对路径，转换为绝对路径
if [[ "$MAP_DIR" != /* ]]; then
    MAP_DIR="$SCRIPT_DIR/$MAP_DIR"
fi

# 创建地图目录（如果不存在）
if [ ! -d "$MAP_DIR" ]; then
    echo "创建地图目录: $MAP_DIR"
    mkdir -p "$MAP_DIR"
fi

# 完整的地图路径
FULL_MAP_PATH="$MAP_DIR/$MAP_NAME"

echo "=========================================="
echo "保存地图"
echo "=========================================="
echo ""
echo "地图路径: $FULL_MAP_PATH"
echo "地图文件: $FULL_MAP_PATH.pgm"
echo "元数据文件: $FULL_MAP_PATH.yaml"
echo ""

# 检查 slam_toolbox 节点是否运行
if ! ros2 node list | grep -q "slam_toolbox"; then
    echo "错误: slam_toolbox 节点未运行"
    echo "请先启动建图系统: ./mapping.sh"
    exit 1
fi

# 调用 slam_toolbox 的地图保存服务
echo "正在保存地图..."
ros2 service call /slam_toolbox/save_map slam_toolbox/srv/SaveMap "{name: {data: '$FULL_MAP_PATH'}}"

ros2 service call /map_save std_srvs/srv/Trigger
# 等待文件生成
sleep 2

# 检查文件是否生成
if [ -f "$FULL_MAP_PATH.pgm" ] && [ -f "$FULL_MAP_PATH.yaml" ]; then
    echo ""
    echo "=========================================="
    echo "地图保存成功！"
    echo "=========================================="
    echo ""
    echo "地图文件:"
    echo "  - $FULL_MAP_PATH.pgm"
    echo "  - $FULL_MAP_PATH.yaml"
    echo ""
    echo "文件大小:"
    ls -lh "$FULL_MAP_PATH.pgm" "$FULL_MAP_PATH.yaml"
    echo ""
    echo "下一步:"
    echo "  1. 可以继续建图，或按 Ctrl+C 退出建图系统"
    echo "  2. 使用地图进行导航:"
    echo "     ./run.sh --indoor --map $MAP_PATH"
    echo ""
else
    echo ""
    echo "=========================================="
    echo "地图保存失败"
    echo "=========================================="
    echo ""
    echo "可能的原因:"
    echo "  1. slam_toolbox 服务调用失败"
    echo "  2. 地图路径权限不足"
    echo "  3. 磁盘空间不足"
    echo ""
    echo "请检查 slam_toolbox 节点日志:"
    echo "  ros2 node list | grep slam_toolbox"
    echo ""
    exit 1
fi
