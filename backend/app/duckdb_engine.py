from __future__ import annotations

from pathlib import Path
from typing import Any

import duckdb


def get_schema(db_path: Path) -> dict[str, Any]:
    if not db_path.exists():
        return {"tables": []}

    conn = duckdb.connect(str(db_path), read_only=True)
    try:
        tables = conn.execute(
            "SELECT table_name FROM information_schema.tables WHERE table_schema = 'main'"
        ).fetchall()
        result = []
        for (table_name,) in tables:
            cols = conn.execute(f"PRAGMA table_info('{table_name}')").fetchall()
            result.append(
                {
                    "name": table_name,
                    "columns": [{"name": c[1], "type": c[2]} for c in cols],
                }
            )
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

        time_column = _detect_time_column(columns)
        value_columns = [c for c in columns if c != time_column and _looks_numeric(rows, columns.index(c))]

        return {
            "columns": columns,
            "rows": [_serialize_row(row) for row in rows],
            "meta": {
                "time_column": time_column,
                "value_columns": value_columns,
                "row_count": len(rows),
            },
        }
    finally:
        conn.close()


def _detect_time_column(columns: list[str]) -> str | None:
    for candidate in ("_time", "time", "timestamp", "ts"):
        if candidate in columns:
            return candidate
    return None


def _time_to_chart_value(val: Any) -> Any:
    if val is None:
        return None
    if isinstance(val, (int, float)) and val > 1e12:
        return float(val) / 1_000_000
    return val


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
