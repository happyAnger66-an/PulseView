# Protobuf 数据源

将 length-delimited 二进制 Protobuf 监控文件导入 DuckDB，用 SQL 查询指标。

## 适用场景

- 非 ROS 的自定义监控落盘（`.pb`）
- PulseView 内置 schema：`pulseview.MetricSample`（host、cpu_percent、mem_percent、per-core usage）

## 前置条件

- 后端已启动
- 文件为 **varint 长度前缀** 串联的 protobuf 消息流（与 `iter_delimited` 解析方式一致）

## 添加数据源

1. **数据源管理** → **新增** → **Protobuf**
2. 填写 **Protobuf 文件路径**，点击 **扫描**
3. 选择 **消息类型**（默认 `pulseview.MetricSample`）
4. **保存并导入 DuckDB**

### 配置项

| settings 键 | 说明 |
|-------------|------|
| `proto.path` | `.pb` 文件绝对路径 |
| `proto.msg_type` | 消息类型名 |

## 数据探索

导入后主要表：

| 表名 | 说明 |
|------|------|
| `proto_metric` | 主表：`_time`、`host`、`cpu_percent`、`mem_percent` |
| `proto_metric_cores` | 子表：每核 `usage`，维度 `core` |

示例 SQL：

```sql
SELECT _time, cpu_percent, mem_percent FROM proto_metric ORDER BY _time;

SELECT _time, core, usage FROM proto_metric_cores ORDER BY _time, core;
```

点击 Schema 字段可自动生成查询；含 `_time` 与默认指标列时显示 **时序图**。

## 生成测试样本

```bash
cd backend
.venv/bin/python scripts/gen_proto_sample.py
# 默认输出 ../test2/proto_sample.pb
```

## 扩展新消息类型

需修改 `proto_schema.py` 与 `proto_importer.py` 中的 flatten 逻辑，或按 [add_new_format.md](../add_new_format.md) 新增独立 importer。
