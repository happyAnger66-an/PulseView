from __future__ import annotations

from dataclasses import dataclass, field
from typing import Any, Protocol


@dataclass
class ColumnDef:
    name: str
    duckdb_type: str


@dataclass
class TableDef:
    name: str
    columns: list[ColumnDef]
    parent_table: str | None = None
    join_key: str = "msg_id"
    dimension_keys: list[str] = field(default_factory=list)
    default_metrics: list[str] = field(default_factory=list)


@dataclass
class IngestContext:
    msg_id: int
    time_us: int


class RosMsgAdapter(Protocol):
    msg_type: str

    def tables(self) -> list[TableDef]: ...

    def flatten(self, msg: dict[str, Any], ctx: IngestContext) -> dict[str, list[dict[str, Any]]]: ...


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
