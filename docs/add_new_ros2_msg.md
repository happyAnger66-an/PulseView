# 新增 ROS2 消息类型

面向需要为 PulseView 添加新 msg 解析与显示的开发者。

## 数据流

```
MCAP → mcap_ros2 解码为 dict → Adapter.flatten() → DuckDB
  → Schema API / SQL 查询 → 时序图 + 统计表（avg / p50 / p99）
```

存储采用 **long format**（子表通过 `msg_id` 关联主表 `_time`），展示层按 `dimension_keys` 自动分线。**图表、pivot、统计等组件无需修改**。

---

## 1. 设计表结构

先规划再写代码：

| 消息部分 | 存储方式 | 示例 |
|---------|---------|------|
| 标量字段 + 时间戳 | **主表**（`msg_id`、`_time`） | `system_stats` |
| 数组字段 | **子表**（`msg_id` + `idx` + 维度 + 指标） | `system_stats_node_pub_stats` |
| 单条嵌套 struct | 独立子表或并入主表 | `system_stats_mem_detail_stat` |

子表在 `TableDef` 中声明：

- `parent_table` — 主表名
- `dimension_keys` — 分线主键，如 `["node", "topic"]`
- `default_metrics` — 默认可视化指标，如 `["hz"]`

---

## 2. 编写 Adapter（必做）

在 `backend/app/ingest/adapters/` 新建文件，参考 [`system_stats.py`](../backend/app/ingest/adapters/system_stats.py)：

```python
from app.ingest.registry import ColumnDef, IngestContext, TableDef, registry

MSG_TYPE = "your_pkg/msg/YourMsg"


def _expand_array(msg_id, items, mapper):
    rows = []
    for idx, item in enumerate(items or []):
        row = mapper(item)
        row["msg_id"] = msg_id
        row["idx"] = idx
        rows.append(row)
    return rows


class YourMsgAdapter:
    msg_type = MSG_TYPE

    def tables(self) -> list[TableDef]:
        return [
            TableDef(
                "your_msg",
                [
                    ColumnDef("msg_id", "BIGINT"),
                    ColumnDef("_time", "BIGINT"),
                    # ... 标量列
                ],
                default_metrics=["some_metric"],
            ),
            TableDef(
                "your_msg_items",
                [
                    ColumnDef("msg_id", "BIGINT"),
                    ColumnDef("idx", "UINTEGER"),
                    ColumnDef("name", "VARCHAR"),
                    ColumnDef("value", "FLOAT"),
                ],
                parent_table="your_msg",
                dimension_keys=["name"],
                default_metrics=["value"],
            ),
        ]

    def flatten(self, msg, ctx: IngestContext) -> dict[str, list[dict]]:
        msg_id = ctx.msg_id
        return {
            "your_msg": [{
                "msg_id": msg_id,
                "_time": ctx.time_us,
                # ... 从 msg 取标量字段
            }],
            "your_msg_items": _expand_array(
                msg_id,
                msg.get("items"),
                lambda x: {"name": x.get("name"), "value": x.get("value")},
            ),
        }


registry.register(YourMsgAdapter())
```

要点：

- `_time` 使用 `ctx.time_us`（微秒），或从 `header.stamp` 转换
- 子表必须含 `msg_id`，与主表 JOIN
- 数组元素用 `_expand_array` 展开并保留 `idx`

---

## 3. 注册 Adapter（必做）

在 [`backend/app/ingest/adapters/__init__.py`](../backend/app/ingest/adapters/__init__.py) 添加 import：

```python
import app.ingest.adapters.your_msg  # noqa: F401
```

未注册会在 ingest 时报 `unsupported msg type`。

---

## 4. 配置数据源并导入

1. **数据源管理** → 添加 ROS2 MCAP 数据源
2. 填写 MCAP 路径、Topic、Msg 类型（与 `MSG_TYPE` 完全一致）
3. 保存后自动 ingest，或手动点「重新导入」

**前提**：MCAP 文件须包含该 msg 的 schema，`mcap-ros2-support` 才能解码。成功后 DuckDB 写入 `backend/data/duckdb/`。

---

## 5. 验证显示

Adapter 注册并导入成功后，前端通常**零改动**即可：

- Schema 树展示新表结构
- 点击数值字段 → 自动生成 JOIN SQL（含维度列）
- 时序图按 `dimension_keys` 分线，下方显示 avg / p50 / p99

也可手写 SQL 验证：

```sql
SELECT s._time, i.name, i.value
FROM your_msg s
JOIN your_msg_items i ON i.msg_id = s.msg_id
ORDER BY s._time, i.name
```

---

## 6. 前端增强（可选）

| 工作 | 文件 | 何时需要 |
|------|------|---------|
| 快捷预设按钮 | `fronted/src/components/SqlGraph/index.tsx` | 常用查询需一键执行 |
| SQL 自动生成通用化 | `fronted/src/components/SqlGraph/sqlBuilder.ts` | 主表名不是 `system_stats` 时（见下方说明） |
| 默认 msg 类型 | `fronted/src/plugins/ros2_mcap/constants.ts` | 改表单默认值 |

### sqlBuilder 限制

当前 `sqlBuilder.ts` 中 `MAIN_TABLE` 硬编码为 `system_stats`，子表 JOIN 也写死该主表。若新 msg 主表名不同，Schema 点击生成的 SQL 会不正确，需改为读取 schema 中的 `parent_table`。

---

## 7. 测试检查清单

- [ ] Ingest 成功：`messages_decoded > 0`
- [ ] `GET /api/datasources/{id}/schema` 返回预期表及 `dimension_keys`
- [ ] 主表标量字段可绘制时序图
- [ ] 子表带维度的指标可按 `(dimension_keys...)` 分线
- [ ] 拖拽选区后统计表数值随之变化
- [ ] 图例显隐与统计表联动

---

## 最小工作量

| 目标 | 所需工作 |
|------|---------|
| 能解析、能查、能看图 | 步骤 1–4（Adapter + 注册 + 导入） |
| Schema 点击生成 SQL、预设按钮 | 额外步骤 6 |

参考实现：[`backend/app/ingest/adapters/system_stats.py`](../backend/app/ingest/adapters/system_stats.py)
