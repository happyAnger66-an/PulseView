# PulseView

监控性能数据的轻量可视化工具。通过可插拔的格式导入器把多种数据（MCAP、Protobuf、CTF）
统一导入 DuckDB 后用 SQL 探索，并按数据形态自动选择折线、泳道（Timeline）或表格展示；
另支持通过 PromQL 查询 SQLite 时序库。

## 支持的数据源类型

| plugin_type | 名称 | 输入 | 能力 (capabilities) | 默认图表 |
|-------------|------|------|---------------------|----------|
| `ros2_mcap` | ROS2 MCAP | MCAP 文件 + Topic + Msg 类型 | ingest / schema / sql | 折线 / 表格 |
| `protobuf` | Protobuf | length-delimited `.pb` 文件 + 消息类型 | ingest / schema / sql | 折线 / 表格 |
| `ctf` | CTF Trace | CTF trace 目录（`metadata` + `stream_*`） | ingest / schema / sql | Timeline / 表格 |
| `sqlite` | SQLite | SQLite 文件路径 | promql | 折线 |

> `ingest/schema/sql` 类数据源导入 DuckDB 后用 SQL 查询；`sqlite` 走 PromQL 查询。
> 能力由后端 `PLUGIN_META` 声明，前端据此显隐 UI。

## 支持的图表类型

图表选择只依赖**查询结果的 `meta`**，与数据来自哪种格式无关：

| viz type | 名称 | 适用条件 (meta) | 说明 |
|----------|------|-----------------|------|
| `timeseries` | 图表 | 有 `time_column` 且有 `value_columns` | uPlot 多线时序图 + 统计表（avg/p50/p99）+ 图例显隐 |
| `timeline` | Timeline | 有 `time_column` 且有 `dur_column` | canvas 泳道（swimlane）渲染，拖拽框选缩放、双击重置、hover tooltip |
| `table` | 表格 | 任意结果 | 原始查询结果表 |

## 快速预览

#### 设置数据源
![设置数据源](./img/data_sources.png)

#### 查看监控指标
![查看监控指标](./img/metrics.png)

## 目录

```
PulseView/
├── backend/     FastAPI + DuckDB + 格式导入器（MCAP / Protobuf / CTF）
├── fronted/     React + Vite + uPlot（折线）+ canvas（Timeline）
└── docs/        架构与扩展文档
```

相关文档：[架构图](docs/architecture.md) · [整体设计](docs/design.md) · [扩展新格式](docs/add_new_format.md) · [扩展 ROS 消息](docs/add_new_ros2_msg.md)

## 后端

```bash
cd backend
pip install -r requirements.txt
python run.py          # http://0.0.0.0:8080
```

主要 API：

| 接口 | 说明 |
|------|------|
| `GET /api/datasource/plugins` | 列出数据源插件类型及能力 |
| `GET/POST /api/datasources` | 数据源 CRUD |
| `GET /api/inspect?plugin_type=&path=` | 扫描原始文件（按格式分派，返回 topic/区间信息） |
| `POST /api/datasources/{id}/ingest` | 触发导入（MCAP / Protobuf / CTF） |
| `GET /api/datasources/{id}/schema` | DuckDB 表结构 + 可视化元数据 |
| `POST /api/sql/query` | 执行 SQL 查询（仅 SELECT） |
| `POST /api/query_range` | PromQL 范围查询（SQLite） |

数据与 DuckDB 文件保存在 `backend/data/`。

## 前端

```bash
cd fronted
npm install
npm run dev            # http://localhost:8766，代理 /api → :8080
```

无后端时可 mock 启动：`USE_MOCK=true npm run dev`

## 使用流程

1. 启动后端与前端
2. **数据源管理** → 添加数据源
   - **ROS2 MCAP**：填写 MCAP 路径、Topic、Msg 类型（如 `system_stats_interfaces/msg/SystemStats`），保存后自动导入
   - **Protobuf**：填写 `.pb` 路径并扫描，选择消息类型，保存后自动导入
   - **CTF Trace**：填写 trace 目录路径并扫描，保存后自动导入
   - **SQLite**：填写数据库路径
3. **数据探索** → 选择数据源
   - DuckDB 类（MCAP/Protobuf/CTF）：左侧 Schema 点字段生成 SQL，或点预设按钮，Ctrl+Enter 执行；结果按形态自动切换折线 / Timeline / 表格
   - SQLite：输入 PromQL 查询

## 扩展

- **新增数据格式**（如新的二进制/trace 格式）：实现 `FormatImporter` + `TableDef`，注册到 `format_registry`，并在 `PLUGIN_META` 声明能力。详见 [docs/add_new_format.md](docs/add_new_format.md)。
- **为 MCAP 新增 ROS 消息类型**：在 `backend/app/ingest/adapters/` 新增 Adapter（声明 `dimension_keys`、`default_metrics`）并注册到 `registry`。详见 [docs/add_new_ros2_msg.md](docs/add_new_ros2_msg.md)。
- **区间/trace → Timeline**：表设 `table_kind="span"` 并含 `_time` + `_dur` 列，前端自动启用泳道视图。

## 测试

测试样本文件默认放在仓库同级的 `../test2/` 目录，可用环境变量覆盖路径：

| 格式 | 样本 | 生成方式 | 环境变量 |
|------|------|----------|----------|
| MCAP | `../test2/test2_0.mcap`（Topic `/slave/system_stats`） | 外部提供 | `PULSEVIEW_TEST_MCAP` |
| Protobuf | `../test2/proto_sample.pb` | `python backend/scripts/gen_proto_sample.py` | `PULSEVIEW_TEST_PROTO` |
| CTF | `../test2/ctf_sample/` | `python backend/scripts/gen_ctf_sample.py` | `PULSEVIEW_TEST_CTF` |

### 后端单元 / 集成测试

```bash
cd backend
python3 -m venv .venv
.venv/bin/pip install -r requirements.txt -r requirements-dev.txt
.venv/bin/python -m pytest tests -q
```

覆盖：Adapter flatten、DuckDB 列推断（含 `_dur`）、`_pv_table_meta` 元数据、各 Importer（MCAP/Protobuf/CTF）的 ingest + Schema + SQL 查询。

### E2E 测试（Playwright）

自动启动隔离后端（`e2e/.data`）与前端 dev server，覆盖 API 与 UI 核心流程。

```bash
# 项目根目录
npm install
npx playwright install chromium   # postinstall 已包含，首次需联网
npm run test:e2e
```

| 用例文件 | 覆盖 |
|---------|------|
| `e2e/tests/backend-api.spec.ts` | 插件列表、MCAP/Protobuf/CTF 导入、Schema 元数据、SQL 查询、非法 SQL 拒绝 |
| `e2e/tests/explorer-ui.spec.ts` | 预设查询、时序图、统计表、图例显隐、表格切换 |
| `e2e/tests/timeline-ui.spec.ts` | CTF span 数据的 Timeline 泳道渲染 |
| `e2e/tests/datasource-ui.spec.ts` | 数据源列表、页面导航 |
