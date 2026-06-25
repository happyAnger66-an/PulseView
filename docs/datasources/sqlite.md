# SQLite 数据源

直接连接已有 SQLite 时序库，通过 **PromQL** 查询并在前端绘制时序图。

## 适用场景

- 兼容 Prometheus 风格查询入口的监控数据
- 不需要 DuckDB 导入流程的轻量时序库

> 与 MCAP / Protobuf / CTF / Perfetto 不同：SQLite 数据源**不经过 ingest**，不走 SQL 探索面板。

## 前置条件

- 后端已启动
- SQLite 文件路径对后端进程可读

## 添加数据源

1. **数据源管理** → **新增** → **SQLite**
2. 填写 **数据库路径**（如 `./data/metrics.db`）
3. 点击 **保存**（无自动导入步骤）

### 配置项

| settings 键 | 说明 |
|-------------|------|
| `sqlite.path` | SQLite 数据库文件路径 |

## 数据探索

1. 进入 **数据探索**，选择该 SQLite 数据源
2. 在 PromQL 输入框输入查询表达式
3. 选择时间范围后执行，结果以 **时序图** 展示

当前后端 `POST /api/query_range` 为 Prometheus 兼容的占位实现，返回模拟时序数据，便于前端联调。接入真实 SQLite 指标库时需扩展该接口。

## API

```http
POST /api/query_range
Content-Type: application/json

{
  "query": "up",
  "start": 0,
  "end": 3600,
  "step": 15
}
```

## 与 DuckDB 类数据源对比

| | SQLite | ros2_mcap / protobuf / ctf / perfetto |
|--|--------|----------------------------------------|
| 查询语言 | PromQL | SQL |
| 存储 | 原始 SQLite 文件 | 导入到 `backend/data/duckdb/{id}.duckdb` |
| 能力 | `promql` | `ingest` + `schema` + `sql` |
| 默认图表 | 时序图 | 时序图 / Timeline / 表格 |
