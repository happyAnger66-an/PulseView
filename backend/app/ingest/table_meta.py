from __future__ import annotations

import json
from typing import Any

import duckdb

from app.ingest.registry import TableDef

# DuckDB 内置元数据表名：ingest 时落库，Schema API 读取以驱动前端可视化，
# 使表结构语义与「是哪种格式导入的」彻底解耦。
META_TABLE = "_pv_table_meta"

_CREATE_SQL = f"""
CREATE TABLE IF NOT EXISTS {META_TABLE} (
    table_name VARCHAR PRIMARY KEY,
    table_kind VARCHAR,
    parent_table VARCHAR,
    join_key VARCHAR,
    dimension_keys VARCHAR,
    default_metrics VARCHAR,
    time_column VARCHAR
)
"""


def write_table_meta(
    conn: duckdb.DuckDBPyConnection,
    tables: list[TableDef],
    time_column: str = "_time",
) -> None:
    """将 TableDef 的可视化元数据写入 ``_pv_table_meta``（dimension_keys/default_metrics 以 JSON 字符串存储）。"""
    conn.execute(_CREATE_SQL)
    conn.execute(f"DELETE FROM {META_TABLE}")
    for t in tables:
        conn.execute(
            f"INSERT INTO {META_TABLE} VALUES (?, ?, ?, ?, ?, ?, ?)",
            [
                t.name,
                t.table_kind,
                t.parent_table,
                t.join_key,
                json.dumps(t.dimension_keys),
                json.dumps(t.default_metrics),
                time_column,
            ],
        )


def read_table_meta(conn: duckdb.DuckDBPyConnection) -> dict[str, dict[str, Any]]:
    """读取 ``_pv_table_meta``，返回 ``{table_name: 元数据}``；表不存在时返回空 dict。"""
    exists = conn.execute(
        "SELECT 1 FROM information_schema.tables "
        "WHERE table_schema = 'main' AND table_name = ?",
        [META_TABLE],
    ).fetchone()
    if not exists:
        return {}

    rows = conn.execute(
        f"SELECT table_name, table_kind, parent_table, join_key, "
        f"dimension_keys, default_metrics, time_column FROM {META_TABLE}"
    ).fetchall()
    meta: dict[str, dict[str, Any]] = {}
    for name, kind, parent, join_key, dims, metrics, time_col in rows:
        meta[name] = {
            "table_kind": kind,
            "parent_table": parent,
            "join_key": join_key,
            "dimension_keys": json.loads(dims) if dims else [],
            "default_metrics": json.loads(metrics) if metrics else [],
            "time_column": time_col,
        }
    return meta
