# PulseView

ROS2 / 时序数据的轻量可视化工具。支持将 MCAP 中的 ROS2 消息导入 DuckDB 后用 SQL 探索，或通过 PromQL 查询 SQLite 时序库。

## 功能

- **数据源管理**：配置 SQLite 或 ROS2 MCAP 数据源
- **ROS2 MCAP**：解析 MCAP（如 `SystemStats`），扁平化写入 DuckDB，按 node/topic 等维度绘制时序曲线
- **SQLite**：PromQL 查询 + 时序图表
- **数据探索**：Schema 树点击生成 SQL、预设查询、表格/图表切换、曲线图例显隐

## 目录

```
PulseView/
├── backend/     FastAPI + DuckDB + MCAP 导入
└── fronted/     React + Vite + uPlot 前端
```

## 后端

```bash
cd backend
pip install -r requirements.txt
python run.py          # http://0.0.0.0:8080
```

主要 API：

| 接口 | 说明 |
|------|------|
| `GET/POST /api/datasources` | 数据源 CRUD |
| `POST /api/datasources/{id}/ingest` | 触发 MCAP 导入 |
| `GET /api/datasources/{id}/schema` | DuckDB 表结构 |
| `POST /api/sql/query` | 执行 SQL 查询 |
| `POST /api/query_range` | PromQL 范围查询 |

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
   - **SQLite**：填写数据库路径
3. **数据探索** → 选择数据源
   - MCAP：左侧 Schema 点字段生成 SQL，或选预设（CPU、Node Pub Hz 等），Ctrl+Enter 执行
   - SQLite：输入 PromQL 查询

## 扩展 ROS 消息类型

在 `backend/app/ingest/adapters/` 新增 Adapter，声明 `TableDef`（含 `dimension_keys`、`default_metrics`），并注册到 `registry`。详见 [docs/add_new_ros2_msg.md](docs/add_new_ros2_msg.md)。

## 测试

测试依赖样本 MCAP：`../test2/test2_0.mcap`（Topic `/slave/system_stats`）。可通过环境变量 `PULSEVIEW_TEST_MCAP` 覆盖路径。

### 后端单元 / 集成测试

```bash
cd backend
python3 -m venv .venv
.venv/bin/pip install -r requirements.txt -r requirements-dev.txt
.venv/bin/python -m pytest tests -q
```

覆盖：Adapter flatten、DuckDB 列推断、数据源 ingest + SQL 查询 API。

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
| `e2e/tests/backend-api.spec.ts` | 插件列表、MCAP 导入、Schema 元数据、SQL 查询、非法 SQL 拒绝 |
| `e2e/tests/explorer-ui.spec.ts` | 预设查询、时序图、统计表、图例显隐、表格切换 |
| `e2e/tests/datasource-ui.spec.ts` | 数据源列表、页面导航 |
