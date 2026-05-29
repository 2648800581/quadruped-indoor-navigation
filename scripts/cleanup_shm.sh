#!/bin/bash

# 清理 Fast-DDS 共享内存文件
# 用于解决 RTPS_TRANSPORT_SHM Error

echo "清理 Fast-DDS 共享内存文件..."

# 清理 /dev/shm 中的 fastrtps 文件
sudo rm -f /dev/shm/fastrtps_*
sudo rm -f /dev/shm/fast_datasharing_*

# 清理 /tmp 中的相关文件
sudo rm -f /tmp/fastrtps_*

# 显示剩余的共享内存段
echo ""
echo "当前共享内存段："
ipcs -m

echo ""
echo "清理完成！"
echo "现在可以重新启动系统了。"
