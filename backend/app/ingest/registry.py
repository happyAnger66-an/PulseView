from __future__ import annotations

from dataclasses import dataclass, field
from typing import Any, Protocol


@dataclass
class ColumnDef:
    name: str
    duckdb_type: str


@dataclass
class TableDef:
    # DuckDB 表名
    name: str
    # 列名与 DuckDB 类型
    columns: list[ColumnDef]
    # 父表名；主表为 None，子表指向承载 _time 的主表
    parent_table: str | None = None
    # 与父表关联的外键列，默认 msg_id
    join_key: str = "msg_id"
    # 时序图分线维度（如 node、topic），空列表表示不分线
    dimension_keys: list[str] = field(default_factory=list)
    # 前端默认可视化的数值指标列
    default_metrics: list[str] = field(default_factory=list)


@dataclass
class IngestContext:
    msg_id: int
    time_us: int


class RosMsgAdapter(Protocol):
    """ROS2 消息适配器：将解码后的 msg dict 展平为 DuckDB 行数据。

    每种 msg 类型实现一个 Adapter，注册到 ``registry`` 后供 ingest 管线与 Schema API 使用。
    """

    # ROS2 消息类型全名，如 system_stats_interfaces/msg/SystemStats
    msg_type: str

    def tables(self) -> list[TableDef]:
        """声明该 msg 对应的 DuckDB 表结构（主表 + 子表）及可视化元数据。"""
        ...

    def flatten(self, msg: dict[str, Any], ctx: IngestContext) -> dict[str, list[dict[str, Any]]]:
        """将单条 ROS 消息展平为多表行批次。

        返回 ``{表名: [行字典, ...]}``；行字典的键须与 ``tables()`` 中对应表的列名一致。
        ``ctx`` 提供全局 ``msg_id`` 与时间戳，用于主表 ``_time`` 及子表外键关联。
        """
        ...


class RosMsgAdapterRegistry:
    def __init__(self) -> None:
        self._adapters: dict[str, RosMsgAdapter] = {}

    def register(self, adapter: RosMsgAdapter) -> None:
        self._adapters[adapter.msg_type] = adapter

    def get(self, msg_type: str) -> RosMsgAdapter:
        if msg_type not in self._adapters:
            raise KeyError(f"unsupported msg type: {msg_type}")
        return self._adapters[msg_type]

    def list_msg_types(self) -> list[str]:
        return list(self._adapters.keys())


registry = RosMsgAdapterRegistry()
