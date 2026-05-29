# 地图目录

此目录用于存储建图过程中创建的地图文件。

## 创建地图

1. 启动建图模式：
   ```bash
   ./mapping.sh
   ```

2. 控制机器人探索环境：
   ```bash
   ros2 run d1_dog_control keyboard_teleop_node
   ```

3. 建图完成后保存地图：
   ```bash
   ./save_map.sh maps/my_map
   ```

## 地图文件

每个地图由两个文件组成：
- `地图名.pgm` - 占据栅格图像
- `地图名.yaml` - 地图元数据（分辨率、原点等）

## 使用地图

加载已保存的地图进行导航：
```bash
./run.sh --map maps/my_map
```
