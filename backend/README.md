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

## Features

- **ros2_mcap** datasource: scan MCAP topics, ingest `SystemStats` into DuckDB
- **SQL query** API for frontend explorer
- Extensible `RosMsgAdapter` registry

## API

| Endpoint | Description |
|----------|-------------|
| `GET /api/mcap/inspect?path=` | List topics in MCAP file |
| `POST /api/datasources` | Create datasource (auto-ingest for ros2_mcap) |
| `POST /api/datasources/:id/ingest` | Re-ingest MCAP |
| `GET /api/datasources/:id/schema` | DuckDB table schema |
| `POST /api/sql/query` | Run SELECT query |

## DuckDB Tables (SystemStats)

- `system_stats` — scalar fields + `_time` (microseconds)
- `system_stats_cpu_stats`, `system_stats_gpu_stats`, `system_stats_proc_stats`, ...
- `system_stats_node_pub_stats`, `system_stats_node_sub_stats` — ROS 节点 pub/sub 统计

Example:

```sql
SELECT _time, cpu_used_percent, mem_used_percent FROM system_stats ORDER BY _time;
```

## Extend new ROS2 msg

1. Add adapter in `app/ingest/adapters/your_msg.py`
2. Register in `registry.register(YourAdapter())`
3. Import in `app/ingest/adapters/__init__.py`
