from __future__ import annotations

from pathlib import Path
from typing import Any

import duckdb

from app.ingest.registry import registry

SKIP_VALUE_COLUMNS = frozenset({"msg_id", "idx", "seq"})
SKIP_VALUE_SUFFIXES = ("_ts",)
KNOWN_DIMENSION_COLUMNS = frozenset({
    "node",
    "topic",
    "name",
    "cpu_name",
    "mount_point",
    "pid",
    "status",
    "frame_id",
    "hostname",
    "vin",
})


def get_schema(db_path: Path, msg_type: str | None = None) -> dict[str, Any]:
    if not db_path.exists():
        return {"tables": []}

    adapter_meta: dict[str, dict[str, Any]] = {}
    if msg_type:
        try:
            adapter = registry.get(msg_type)
            for table in adapter.tables():
                adapter_meta[table.name] = {
                    "parent_table": table.parent_table,
                    "join_key": table.join_key,
                    "dimension_keys": table.dimension_keys,
                    "default_metrics": table.default_metrics,
                }
        except KeyError:
            pass

    conn = duckdb.connect(str(db_path), read_only=True)
    try:
        tables = conn.execute(
            "SELECT table_name FROM information_schema.tables WHERE table_schema = 'main'"
        ).fetchall()
        result = []
        for (table_name,) in tables:
            cols = conn.execute(f"PRAGMA table_info('{table_name}')").fetchall()
            entry: dict[str, Any] = {
                "name": table_name,
                "columns": [{"name": c[1], "type": c[2]} for c in cols],
            }
            entry.update(adapter_meta.get(table_name, {}))
            result.append(entry)
        return {"tables": result}
    finally:
        conn.close()


def run_query(db_path: Path, sql: str, limit: int = 1000) -> dict[str, Any]:
    if not db_path.exists():
        raise FileNotFoundError("duckdb file not found, please ingest first")

    normalized = sql.strip().rstrip(";")
    if not normalized.lower().startswith("select"):
        raise ValueError("only SELECT queries are allowed")

    conn = duckdb.connect(str(db_path), read_only=True)
    try:
        wrapped = f"SELECT * FROM ({normalized}) AS _q LIMIT {limit}"
        cur = conn.execute(wrapped)
        columns = [d[0] for d in cur.description]
        rows = cur.fetchall()

        time_column, dimension_columns, value_columns = _infer_columns(columns, rows)

        return {
            "columns": columns,
            "rows": [_serialize_row(row) for row in rows],
            "meta": {
                "time_column": time_column,
                "dimension_columns": dimension_columns,
                "value_columns": value_columns,
                "row_count": len(rows),
            },
        }
    finally:
        conn.close()


def _infer_columns(
    columns: list[str],
    rows: list[tuple],
) -> tuple[str | None, list[str], list[str]]:
    time_column = _detect_time_column(columns)
    dimension_columns: list[str] = []
    value_columns: list[str] = []

    for col in columns:
        if col == time_column or col in SKIP_VALUE_COLUMNS:
            continue
        idx = columns.index(col)
        if col in KNOWN_DIMENSION_COLUMNS or not _looks_numeric(rows, idx):
            dimension_columns.append(col)
        elif col.endswith(SKIP_VALUE_SUFFIXES):
            continue
        else:
            value_columns.append(col)

    return time_column, dimension_columns, value_columns


def _detect_time_column(columns: list[str]) -> str | None:
    for candidate in ("_time", "time", "timestamp", "ts"):
        if candidate in columns:
            return candidate
    return None


def _looks_numeric(rows: list[tuple], idx: int) -> bool:
    for row in rows[:20]:
        val = row[idx]
        if val is None:
            continue
        if isinstance(val, (int, float)):
            return True
        try:
            float(val)
            return True
        except (TypeError, ValueError):
            return False
    return False


def _serialize_row(row: tuple) -> list[Any]:
    out: list[Any] = []
    for val in row:
        if hasattr(val, "isoformat"):
            out.append(val.isoformat())
        elif isinstance(val, int) and val > 1_000_000_000_000:
            out.append(val / 1_000_000)
        else:
            out.append(val)
    return out
