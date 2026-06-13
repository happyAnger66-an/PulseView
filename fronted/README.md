# PV Fronted

参考 n9e-fe 架构的监控前端，支持多数据源插件。

## 数据源插件

| 插件 | 查询语言 | 能力 | 说明 |
|------|----------|------|------|
| `sqlite` | PromQL | promql | 时序指标查询 |
| `ros2_mcap` | SQL | ingest/schema/sql | MCAP → DuckDB，查询 SystemStats 等 ROS2 msg |

每个插件在 `src/plugins/{type}/index.tsx` 一次性导出 `DatasourceForm` + `QueryPanel`，并在
`src/plugins/index.ts` 声明 `capabilities` 与 `defaultVisualizations`，由 `pages/explorer` 按
`plugin.QueryPanel` 分发，无需在页面里 `if (type === ...)`。

## 可视化注册表

`src/visualizations/` 是与数据源解耦的图表注册表：每种图表声明 `accepts(meta)` 判断是否适用，
`SqlGraph` 用 `selectViz(result.meta)` 动态生成 tab。新增图表（如 timeline 泳道图）只需 `registerViz`，
查询/数据源侧零改动。

| 图表 type | 适用条件 |
|-----------|----------|
| `timeseries` | 结果含 `_time` 且有数值列 |
| `table` | 始终适用 |

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
├── plugins/            # 数据源插件注册表（Form + QueryPanel + 能力声明）
│   ├── sqlite/         # SQLite + PromQL
│   └── ros2_mcap/      # MCAP + DuckDB + SQL
├── visualizations/     # 图表注册表（与数据源解耦）
│   ├── registry.ts     # registerViz / selectViz
│   ├── TimeseriesViz.tsx
│   └── TableViz.tsx
├── components/
│   ├── PromGraph/      # PromQL 探索
│   └── SqlGraph/       # SQL 探索 + Schema 树（viz 注册表驱动 tab）
└── pages/
    ├── explorer/       # 按 plugin.QueryPanel 分发
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
