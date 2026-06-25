# CTF Trace 数据源

将 Common Trace Format（CTF）trace 目录导入 DuckDB，将回调区间转为 span 表，用 **Timeline 泳道图** 查看。

## 适用场景

| 来源 | 说明 |
|------|------|
| 内置最小 CTF | 测试样本，无额外依赖 |
| LTTng / `ros2 trace` | ROS 2 C++ 节点回调 trace，需系统 `bt2` |

## 前置条件

- 后端已启动
- **CTF trace 目录**：含 `metadata` 与 `stream_*` 子目录（或 LTTng session 目录）
- 读取真实 LTTng trace 时需安装 babeltrace2：

```bash
sudo apt install babeltrace2 python3-bt2
cd backend
python3 -m venv --system-site-packages .venv
.venv/bin/pip install -r requirements.txt
```

## 添加数据源

1. **数据源管理** → **新增** → **CTF Trace**
2. 填写 **CTF trace 目录** 绝对路径，点击 **扫描**
3. 确认扫描结果中的泳道数量，**保存并导入 DuckDB**

### 配置项

| settings 键 | 说明 |
|-------------|------|
| `ctf.path` | CTF trace 根目录 |

## 数据探索

导入后写入 `ctf_spans` 表（`table_kind=span`），列含 `_time`、`_dur`、`track`、`name` 等。

示例 SQL：

```sql
SELECT _time, _dur, track, name FROM ctf_spans ORDER BY _time;
```

查询结果含 `_time` 与 `_dur` 时自动使用 **Timeline** 泳道图，支持框选缩放与 hover。

## 录制 ROS 2 trace（LTTng）

一键示例（需 ROS 2 Jazzy + lttng-tools）：

```bash
cd samples/ros2_trace_demo
./record_and_verify.sh
```

trace 输出：`samples/ros2_trace_demo/traces/pv_trace_demo`。将该目录填为 `ctf.path` 即可。

> **注意**：callback span 来自 **rclcpp（C++）** 的 `ros2:callback_*` 事件；纯 Python `rclpy` 节点不会产生可配对的 span。

详细原理与手动录制步骤见 [ros2_tracing.md](../ros2_tracing.md)。

## 生成内置测试样本

```bash
cd backend
.venv/bin/python scripts/gen_ctf_sample.py
# 默认输出 ../test2/ctf_sample/
```

## 故障排查

| 现象 | 处理 |
|------|------|
| ingest 提示缺少 `bt2` | 安装 `python3-bt2` 并用 `--system-site-packages` 重建 venv |
| 扫描到 0 个区间 | 检查路径是否为 trace 根目录；LTTng 需含 `ros2:callback_*` 事件 |
| 旧错误状态未更新 | Query 面板 **重新 ingest** |
