# PulseView Backend

FastAPI + DuckDB + ROS2 MCAP ingest service.

## Setup

```bash
cd backend
python3 -m venv .venv
source .venv/bin/activate
pip install -r requirements.txt
python run.py
```

API: http://localhost:8080

### Perfetto 数据源（可选）

除 `pip install perfetto` 外，还需系统中有 `trace_processor_shell`（`which trace_processor_shell`）。诊断：

```bash
source .venv/bin/activate
python -c "from app.ingest import perfetto_tp; print(perfetto_tp.missing_dependency() or 'OK')"
```

详见 `docs/support_perfetto.md`。

## Features

- **ros2_mcap** datasource: scan MCAP topics, ingest `SystemStats` into DuckDB
- **SQL query** API for frontend explorer
- 两级扩展点：格式级 `FormatImporter` 注册表（`app/ingest/importer.py`）+ 消息级 `RosMsgAdapter` 注册表
- 插件能力声明（`PLUGIN_META[*].capabilities`），API 层按能力分派而非硬编码 plugin_type
- ingest 时把可视化元数据落库到 `_pv_table_meta`，Schema API 据此驱动前端图表

## API

| Endpoint | Description |
|----------|-------------|
| `GET /api/inspect?plugin_type=&path=` | 通用文件扫描（按格式分派）；`GET /api/mcap/inspect?path=` 为兼容别名 |
| `POST /api/datasources` | Create datasource (auto-ingest if plugin has `ingest` capability) |
| `POST /api/datasources/:id/ingest` | Re-ingest via FormatImporter |
| `GET /api/datasources/:id/schema` | DuckDB table schema（含 `_pv_table_meta` 元数据） |
| `POST /api/sql/query` | Run SELECT query |

## DuckDB Tables (SystemStats)

- `system_stats` — scalar fields + `_time` (microseconds)
- `system_stats_cpu_stats`, `system_stats_gpu_stats`, `system_stats_proc_stats`, ...
- `system_stats_node_pub_stats`, `system_stats_node_sub_stats` — ROS 节点 pub/sub 统计

Example:

```sql
SELECT _time, cpu_used_percent, mem_used_percent FROM system_stats ORDER BY _time;
```

## Extend new ROS2 msg（同一 MCAP 格式下新增消息类型）

1. Add adapter in `app/ingest/adapters/your_msg.py`
2. Register in `registry.register(YourAdapter())`
3. Import in `app/ingest/adapters/__init__.py`

## Extend new format（新增数据格式，如 Protobuf / CTF）

1. 实现 `FormatImporter`（`format_type` / `required_settings` / `inspect` / `ingest`）于 `app/ingest/importers/your_format.py`
2. `format_registry.register(YourImporter())`，并在 `app/ingest/importers/__init__.py` import
3. 在 `app/store.py` 的 `PLUGIN_META` 增加该 plugin 的展示名与 `capabilities`
4. ingest 时调用 `write_table_meta(conn, tables)` 落库可视化元数据（`table_kind` 选 `timeseries`/`span`/`log`）
