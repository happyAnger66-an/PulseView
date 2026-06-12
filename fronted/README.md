# PV Fronted

参考 n9e-fe 架构的监控前端，支持多数据源插件。

## 数据源插件

| 插件 | 查询语言 | 说明 |
|------|----------|------|
| `sqlite` | PromQL | 时序指标查询 |
| `ros2_mcap` | SQL | MCAP → DuckDB，查询 SystemStats 等 ROS2 msg |

## 开发（推荐：对接 Python 后端）

**终端 1 — 后端**

```bash
cd ../backend
python3 -m venv .venv && source .venv/bin/activate
pip install -r requirements.txt
python run.py
```

**终端 2 — 前端**

```bash
cd fronted
npm install
npm run dev
```

- 前端: http://localhost:8766
- 后端: http://localhost:8080（vite 默认代理 `/api`）

### 启动报 ENOSPC（file watchers 耗尽）

Linux 上同时打开很多项目时可能触发。`npm run dev` 已默认启用 **CHOKIDAR polling** 规避。

永久提高系统上限（可选）：

```bash
echo fs.inotify.max_user_watches=524288 | sudo tee -a /etc/sysctl.conf
sudo sysctl -p
```

关闭 polling：`VITE_USE_POLLING=false npm run dev`（需 inotify 上限足够）

## 仅前端 Mock 模式

```bash
USE_MOCK=true npm run dev
```

## 目录结构

```
src/
├── plugins/
│   ├── sqlite/         # SQLite + PromQL
│   └── ros2_mcap/      # MCAP + DuckDB + SQL
├── components/
│   ├── PromGraph/      # PromQL 探索
│   └── SqlGraph/       # SQL 探索 + Schema 树
└── pages/
    ├── explorer/       # 按插件类型切换查询 UI
    └── datasources/
```

## ROS2 MCAP 使用流程

1. 添加 **ROS2 MCAP** 数据源，填写 `.mcap` 路径
2. 点击 **扫描 Topic**，选择含 `SystemStats` 的 topic
3. 保存后自动 ingest 到 DuckDB
4. 在 **数据探索** 页用 SQL 查询，例如：

```sql
SELECT _time, cpu_used_percent, mem_used_percent FROM system_stats ORDER BY _time;
```

`_time` 为微秒时间戳，前端图表会自动转换为秒。
