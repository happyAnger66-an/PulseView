# 新增数据格式（FormatImporter）

面向需要为 PulseView 接入**新数据格式**（如 Protobuf、CTF、Parquet…）的开发者。
若只是为现有 MCAP 增加新的 ROS2 消息类型，请看 [`add_new_ros2_msg.md`](./add_new_ros2_msg.md)。

## 两级扩展点

PulseView 的 ingest 分两层：

| 层级 | 抽象 | 注册表 | 职责 |
|------|------|--------|------|
| 格式级 | `FormatImporter` | `format_registry` | 打开文件、解码出消息流、展平写 DuckDB |
| 消息级 | Adapter（如 `RosMsgAdapter`） | importer 内部二级注册表 | 单条消息 → 表行（可选，按需引入） |

新增一种文件格式 = 实现一个 `FormatImporter`。MCAP 走 `McapImporter` + `RosMsgAdapter` 两级；
Protobuf 当前内置单一消息映射（无需二级注册表），是最小可参考实现。

## 数据流

```
文件 → FormatImporter.ingest() 解码消息流 → 展平为 long-format 行 → DuckDB
  → 写 _pv_table_meta（表结构 + 可视化元数据）
  → Schema API / SQL 查询 → 时序图 + 统计表
```

只要落库的表带 `_time` 与（可选）`dimension_keys`，**前端图表零改动**即可复用。

---

## 1. 实现 FormatImporter

`FormatImporter` 协议见 [`backend/app/ingest/importer.py`](../backend/app/ingest/importer.py)：

```python
class FormatImporter(Protocol):
    format_type: str                       # 与 plugin_type 一致，如 "protobuf"
    def required_settings(self) -> list[str]: ...   # 必填 settings 键
    def inspect(self, path: str) -> dict: ...        # 扫描文件，返回可选项（topics/消息数等）
    def ingest(self, db_path: Path, settings: dict) -> dict: ...  # 解码 + 写 DuckDB
```

参考最简实现 [`backend/app/ingest/importers/proto_importer.py`](../backend/app/ingest/importers/proto_importer.py)：

```python
from app.ingest.importer import format_registry
from app.ingest.registry import ColumnDef, TableDef
from app.ingest.table_meta import write_table_meta

class ProtobufImporter:
    format_type = "protobuf"

    def required_settings(self) -> list[str]:
        return ["proto.path", "proto.msg_type"]

    def inspect(self, path: str) -> dict:
        # 返回 {"topics": [{"name", "msg_type", "message_count"}]}，供前端表单展示/选择
        ...

    def ingest(self, db_path, settings):
        tables = self._tables()                 # 见第 2 节
        conn = duckdb.connect(str(db_path))
        for t in tables:
            conn.execute(f"CREATE TABLE IF NOT EXISTS {t.name} (...)")
            conn.execute(f"DELETE FROM {t.name}")
        write_table_meta(conn, tables)          # 关键：落可视化元数据
        for msg in self._decode(settings):      # 逐条消息
            for table, rows in self._flatten(msg).items():
                conn.executemany(f"INSERT INTO {table} ...", rows)
        return {"messages_decoded": n}

format_registry.register(ProtobufImporter())
```

要点：

- `_time` 用整数（微秒）便于排序与缩放
- 子表必须含 `msg_id`，与主表 JOIN（long format）
- **务必调用 `write_table_meta`**，否则 schema 缺少 `parent_table`/`dimension_keys`/`default_metrics`

---

## 2. 设计表结构（TableDef）

与 ROS2 消息一致，用 `TableDef` 声明（[`registry.py`](../backend/app/ingest/registry.py)）：

| 消息部分 | 存储 | 关键字段 |
|---------|------|---------|
| 标量 + 时间戳 | **主表** | `msg_id`、`_time`、标量列；`default_metrics` |
| 数组/重复字段 | **子表** | `msg_id`、`idx`、维度列、指标列；`parent_table`、`dimension_keys`、`default_metrics` |

Protobuf 示例（`pulseview.MetricSample`）：主表 `proto_metric`（cpu/mem），子表 `proto_metric_cores`
（按 `core` 分线，指标 `usage`）。

---

## 3. 注册 Importer

在 [`backend/app/ingest/importers/__init__.py`](../backend/app/ingest/importers/__init__.py) 添加：

```python
import app.ingest.importers.proto_importer  # noqa: F401
```

---

## 4. 声明插件能力

在 [`backend/app/store.py`](../backend/app/store.py) 的 `PLUGIN_META` 增加条目：

```python
"protobuf": {
    "plugin_type_name": "Protobuf",
    "category": "protobuf",
    "capabilities": ["ingest", "schema", "sql"],
},
```

`capabilities` 决定 API 门禁与前端 UI 显隐，无需再写 `plugin_type` 字符串比较。

---

## 5. 前端插件（让 UI 可用）

在 `fronted/src/plugins/<type>/` 新建：

| 文件 | 作用 |
|------|------|
| `constants.ts` | `PLUGIN_TYPE` / `PLUGIN_NAME` / 默认值 |
| `Form.tsx` | 建源表单，可调用 `inspectSource(path, type)` 扫描文件 |
| `QueryPanel.tsx` | 查询面板；SQL 类格式直接复用 `<SqlGraph>` |
| `index.tsx` | 导出 `DatasourceForm` / `QueryPanel` / 常量 |

然后在 [`fronted/src/plugins/index.ts`](../fronted/src/plugins/index.ts) 注册：

```ts
[PROTOBUF_TYPE]: {
  type: PROTOBUF_TYPE,
  name: PROTOBUF_NAME,
  queryLanguage: 'sql',
  capabilities: ['ingest', 'schema', 'sql'],
  defaultVisualizations: ['timeseries', 'table'],
  DatasourceForm: ProtobufForm,
  QueryPanel: ProtobufQueryPanel,
},
```

参考 [`fronted/src/plugins/protobuf/`](../fronted/src/plugins/protobuf/)。复用 SQL + 现有图表时，**无需新增可视化组件**。

---

## 6. 生成测试数据 & 验证

Protobuf 提供了生成器 [`backend/scripts/gen_proto_sample.py`](../backend/scripts/gen_proto_sample.py)：

```bash
cd backend && .venv/bin/python scripts/gen_proto_sample.py   # 默认写 ../test2/proto_sample.pb
```

测试清单：

- [ ] 后端单测：`pytest tests -q`（参考 [`test_proto_importer.py`](../backend/tests/test_proto_importer.py)）
- [ ] `format_registry.has("<type>")` 为真
- [ ] Ingest 成功：`messages_decoded > 0`
- [ ] `GET /api/datasources/{id}/schema` 返回预期表及 `dimension_keys`
- [ ] 主表标量可绘制时序图，子表带维度可分线
- [ ] e2e：`npx playwright test -c e2e/playwright.config.cjs backend-api`

---

---

## 7. span 类数据与 Timeline 泳道视图

区间/trace 类数据（如 CTF、perfetto slice）用 **span 表** 表达，前端自动切换到 Timeline 泳道视图：

- `TableDef.table_kind = "span"`
- 表须含 `_time`（区间起点）与 `_dur`（区间时长）两列
- `dimension_keys` 的第一列作为**泳道（track）**分组；若有 `name` 列则用作区间标签与配色
- 后端 `run_query` 检测到 `_dur` 列后在 meta 透出 `dur_column`，前端 `TimelineViz.accepts` 据此启用

时间单位建议用**相对起点的纳秒**且保持 < 1e12，避免被 `_serialize_row` 的 epoch 启发式（>1e12 → /1e6）改写。

CTF 参考实现把 `callback_start/end` 事件按 tid 栈配对成 span：

```python
TableDef(
    name="ctf_spans",
    columns=[ColumnDef("span_id", "BIGINT"), ColumnDef("_time", "BIGINT"),
             ColumnDef("_dur", "BIGINT"), ColumnDef("track", "VARCHAR"),
             ColumnDef("name", "VARCHAR"), ColumnDef("cpu_id", "UINTEGER")],
    table_kind="span",
    dimension_keys=["track"],
    default_metrics=["_dur"],
)
```

生成 CTF 测试 trace：

```bash
cd backend && .venv/bin/python scripts/gen_ctf_sample.py   # 默认写 ../test2/ctf_sample/
```

环境若无 babeltrace2/lttng，可参考 [`ctf_format.py`](../backend/app/ingest/ctf_format.py) 的纯 Python 最小 CTF 1.8 读写；生产环境解析任意 CTF 应改用 babeltrace2。

---

## 最小工作量

| 目标 | 所需工作 |
|------|---------|
| 能解析、能查、能看图 | 第 1–4 节（Importer + TableDef + 注册 + 能力） |
| UI 内可建源/查询 | 额外第 5 节（前端插件） |
| 区间/trace → Timeline 泳道 | 表用 `table_kind="span"` + `_time`/`_dur`（第 7 节） |

完整参考实现：

- 时序（Protobuf）：[`proto_importer.py`](../backend/app/ingest/importers/proto_importer.py) · [`proto_schema.py`](../backend/app/ingest/proto_schema.py) · [`fronted/src/plugins/protobuf/`](../fronted/src/plugins/protobuf/)
- span/Timeline（CTF）：[`ctf_importer.py`](../backend/app/ingest/importers/ctf_importer.py) · [`ctf_format.py`](../backend/app/ingest/ctf_format.py) · [`TimelineViz.tsx`](../fronted/src/visualizations/TimelineViz.tsx) · [`fronted/src/plugins/ctf/`](../fronted/src/plugins/ctf/)
