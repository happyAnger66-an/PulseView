# ROS2 MCAP 数据源

将 ROS 2 录制的 MCAP 文件导入 DuckDB，用 SQL 查询并自动绘制时序图或表格。

## 适用场景

- 车辆/机器人日志中的 ROS 2 bag（MCAP 格式）
- 监控类消息，如 `system_stats_interfaces/msg/SystemStats`（CPU、内存等）

## 前置条件

- 后端已启动（见 [README](../../README.md#快速启动)）
- MCAP 文件路径对**后端进程**可读（填绝对路径最稳妥）

## 添加数据源

1. 打开前端 **数据源管理** → **新增** → 选择 **ROS2 MCAP**
2. 填写 **名称**
3. 填写 **MCAP 文件路径**，点击 **扫描 Topic**
4. 选择 **Topic** 与 **Msg 类型**（当前内置 `system_stats_interfaces/msg/SystemStats`）
5. 点击 **保存并导入 DuckDB**

### 配置项

| settings 键 | 说明 |
|-------------|------|
| `mcap.path` | MCAP 文件绝对路径 |
| `mcap.topic` | 要导入的 topic 名 |
| `mcap.msg_type` | ROS 消息类型全名 |

## 数据探索

1. 进入 **数据探索**，选择该数据源
2. 左侧 **Schema** 树展示 DuckDB 表（如 `system_stats` 及子表）
3. 点击字段或 **预设查询** 生成 SQL，执行后自动选择图表：
   - 含 `_time` + 数值列 → **时序图**
   - 其它 → **表格**

示例 SQL：

```sql
SELECT _time, cpu_percent, mem_percent FROM system_stats ORDER BY _time;
```

## 重新导入

修改 MCAP 路径或 topic 后保存会自动 ingest。也可在 Query 面板点击 **重新 ingest**（`force: true`）。

## 扩展新消息类型

若 MCAP 内为其它 ROS 消息，需在后端注册 `RosMsgAdapter`。见 [add_new_ros2_msg.md](../add_new_ros2_msg.md)。

## 测试样本

```bash
# 默认路径 ../test2/test2_0.mcap，可用环境变量 PULSEVIEW_TEST_MCAP 覆盖
cd backend && .venv/bin/python -m pytest tests/test_mcap_importer.py -q
```
