from __future__ import annotations

from pathlib import Path
from typing import Any

import duckdb

from app.ingest.importer import format_registry
from app.ingest.proto_schema import MSG_TYPE, get_message_class, iter_delimited
from app.ingest.registry import ColumnDef, TableDef
from app.ingest.table_meta import write_table_meta

FORMAT_TYPE = "protobuf"

MAIN_TABLE = "proto_metric"
CORES_TABLE = "proto_metric_cores"


def _tables() -> list[TableDef]:
    """DuckDB 表结构 + 可视化元数据（与 MCAP adapter 同一套 TableDef 抽象）。"""
    return [
        TableDef(
            name=MAIN_TABLE,
            columns=[
                ColumnDef("msg_id", "BIGINT"),
                ColumnDef("_time", "BIGINT"),
                ColumnDef("host", "VARCHAR"),
                ColumnDef("cpu_percent", "DOUBLE"),
                ColumnDef("mem_percent", "DOUBLE"),
            ],
            default_metrics=["cpu_percent", "mem_percent"],
        ),
        TableDef(
            name=CORES_TABLE,
            columns=[
                ColumnDef("msg_id", "BIGINT"),
                ColumnDef("idx", "UINTEGER"),
                ColumnDef("core", "VARCHAR"),
                ColumnDef("usage", "DOUBLE"),
            ],
            parent_table=MAIN_TABLE,
            dimension_keys=["core"],
            default_metrics=["usage"],
        ),
    ]


def _flatten(msg, msg_id: int) -> dict[str, list[dict[str, Any]]]:
    main_row = {
        "msg_id": msg_id,
        "_time": int(msg.time_us),
        "host": msg.host,
        "cpu_percent": msg.cpu_percent,
        "mem_percent": msg.mem_percent,
    }
    core_rows = [
        {"msg_id": msg_id, "idx": idx, "core": c.core, "usage": c.usage}
        for idx, c in enumerate(msg.cores)
    ]
    return {MAIN_TABLE: [main_row], CORES_TABLE: core_rows}


class ProtobufImporter:
    """Protobuf 监控数据导入器。

    读取 length-delimited 的 ``pulseview.MetricSample`` 消息流，展平后写入 DuckDB，
    复用与 MCAP 完全一致的 TableDef / _pv_table_meta / 列语义推断，因此前端时序图表零改动。
    消息级扩展可在此基础上引入 ProtoMsgAdapter 二级注册表。
    """

    format_type = FORMAT_TYPE

    def required_settings(self) -> list[str]:
        return ["proto.path", "proto.msg_type"]

    def inspect(self, path: str) -> dict[str, Any]:
        p = Path(path)
        if not p.exists():
            raise FileNotFoundError(f"proto file not found: {path}")
        count = sum(1 for _ in iter_delimited(p.read_bytes(), get_message_class()))
        return {
            "topics": [
                {"name": MSG_TYPE, "msg_type": MSG_TYPE, "message_count": count},
            ]
        }

    def ingest(self, db_path: Path, settings: dict[str, Any]) -> dict[str, Any]:
        path = Path(settings["proto.path"])
        msg_type = settings["proto.msg_type"]
        if not path.exists():
            raise FileNotFoundError(f"proto file not found: {path}")

        message_class = get_message_class(msg_type)
        tables = _tables()
        conn = duckdb.connect(str(db_path))
        try:
            for t in tables:
                cols = ", ".join(f"{c.name} {c.duckdb_type}" for c in t.columns)
                conn.execute(f"CREATE TABLE IF NOT EXISTS {t.name} ({cols})")
                conn.execute(f"DELETE FROM {t.name}")
            write_table_meta(conn, tables)

            msg_id = 0
            decoded = 0
            for msg in iter_delimited(path.read_bytes(), message_class):
                msg_id += 1
                for table_name, rows in _flatten(msg, msg_id).items():
                    if not rows:
                        continue
                    cols = list(rows[0].keys())
                    placeholders = ", ".join(["?"] * len(cols))
                    col_sql = ", ".join(cols)
                    conn.executemany(
                        f"INSERT INTO {table_name} ({col_sql}) VALUES ({placeholders})",
                        [tuple(r.get(c) for c in cols) for r in rows],
                    )
                decoded += 1
            return {"messages_decoded": decoded, "msg_type": msg_type}
        finally:
            conn.close()


format_registry.register(ProtobufImporter())
