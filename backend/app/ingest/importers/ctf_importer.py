from __future__ import annotations

from pathlib import Path
from typing import Any

import duckdb

from app.ingest import ctf_format
from app.ingest.importer import format_registry
from app.ingest.registry import ColumnDef, TableDef
from app.ingest.table_meta import write_table_meta

FORMAT_TYPE = "ctf"
SPAN_TABLE = "ctf_spans"


def _tables() -> list[TableDef]:
    """span 类表：_time（起点）+ _dur（区间）驱动前端 timeline 泳道视图。"""
    return [
        TableDef(
            name=SPAN_TABLE,
            columns=[
                ColumnDef("span_id", "BIGINT"),
                ColumnDef("_time", "BIGINT"),
                ColumnDef("_dur", "BIGINT"),
                ColumnDef("track", "VARCHAR"),
                ColumnDef("name", "VARCHAR"),
                ColumnDef("cpu_id", "UINTEGER"),
            ],
            table_kind="span",
            dimension_keys=["track"],
            default_metrics=["_dur"],
        ),
    ]


def _build_spans(trace_dir: Path) -> list[dict[str, Any]]:
    """按 tid 用栈配对 callback_start/end，生成 span 行；时间归一到 trace 起点。"""
    pending: dict[int, list[dict]] = {}
    spans: list[dict[str, Any]] = []
    for ev in ctf_format.iter_events(trace_dir):
        if ev["id"] == ctf_format.EV_CALLBACK_START:
            pending.setdefault(ev["tid"], []).append(ev)
        else:
            stack = pending.get(ev["tid"])
            if not stack:
                continue  # 没有匹配 start 的 end，跳过
            start = stack.pop()
            spans.append(
                {
                    "_time": start["ts"],
                    "_dur": ev["ts"] - start["ts"],
                    "track": f"thread-{start['tid']}",
                    "name": start.get("name", ""),
                    "cpu_id": start.get("cpu_id", 0),
                }
            )

    if not spans:
        return spans
    base = min(s["_time"] for s in spans)
    for i, s in enumerate(sorted(spans, key=lambda x: x["_time"])):
        s["span_id"] = i
        s["_time"] -= base  # 相对 trace 起点的纳秒，避免被当作 epoch 时间转换
    return spans


class CtfImporter:
    """CTF（ROS2 tracing）导入器。

    解析 CTF stream → 配对回调区间为 span → 写入 span 类表。复用统一的 TableDef /
    _pv_table_meta 机制；表标记 table_kind=span，前端据此选择 timeline 泳道视图。
    本实现内置最小 CTF 解析（见 ctf_format），生产可替换为 babeltrace2。
    """

    format_type = FORMAT_TYPE

    def required_settings(self) -> list[str]:
        return ["ctf.path"]

    def inspect(self, path: str) -> dict[str, Any]:
        trace_dir = Path(path)
        if not trace_dir.exists():
            raise FileNotFoundError(f"ctf trace not found: {path}")
        spans = _build_spans(trace_dir)
        tracks = sorted({s["track"] for s in spans})
        return {
            "topics": [
                {"name": t, "msg_type": t, "message_count": sum(1 for s in spans if s["track"] == t)}
                for t in tracks
            ]
        }

    def ingest(self, db_path: Path, settings: dict[str, Any]) -> dict[str, Any]:
        trace_dir = Path(settings["ctf.path"])
        if not trace_dir.exists():
            raise FileNotFoundError(f"ctf trace not found: {trace_dir}")

        spans = _build_spans(trace_dir)
        tables = _tables()
        conn = duckdb.connect(str(db_path))
        try:
            for t in tables:
                cols = ", ".join(f"{c.name} {c.duckdb_type}" for c in t.columns)
                conn.execute(f"CREATE TABLE IF NOT EXISTS {t.name} ({cols})")
                conn.execute(f"DELETE FROM {t.name}")
            write_table_meta(conn, tables)

            if spans:
                cols = ["span_id", "_time", "_dur", "track", "name", "cpu_id"]
                placeholders = ", ".join(["?"] * len(cols))
                conn.executemany(
                    f"INSERT INTO {SPAN_TABLE} ({', '.join(cols)}) VALUES ({placeholders})",
                    [tuple(s[c] for c in cols) for s in spans],
                )
            return {"messages_decoded": len(spans), "spans": len(spans)}
        finally:
            conn.close()


format_registry.register(CtfImporter())
