from __future__ import annotations

from pathlib import Path
from typing import Any

import duckdb
from mcap.reader import make_reader
from mcap_ros2.decoder import DecoderFactory

from app.ingest.registry import IngestContext, registry
from app.ingest.ros_msg_utils import ros_msg_to_dict
from app.ingest.table_meta import write_table_meta


def create_schema(conn: duckdb.DuckDBPyConnection, msg_type: str) -> None:
    adapter = registry.get(msg_type)
    for table in adapter.tables():
        cols = ", ".join(f"{c.name} {c.duckdb_type}" for c in table.columns)
        conn.execute(f"CREATE TABLE IF NOT EXISTS {table.name} ({cols})")


def clear_tables(conn: duckdb.DuckDBPyConnection, msg_type: str) -> None:
    adapter = registry.get(msg_type)
    for table in adapter.tables():
        conn.execute(f"DELETE FROM {table.name}")


def ingest_mcap(
    db_path: Path,
    mcap_path: Path,
    topic: str,
    msg_type: str,
) -> dict[str, Any]:
    if not mcap_path.exists():
        raise FileNotFoundError(f"mcap file not found: {mcap_path}")

    adapter = registry.get(msg_type)
    conn = duckdb.connect(str(db_path))
    try:
        create_schema(conn, msg_type)
        clear_tables(conn, msg_type)
        write_table_meta(conn, adapter.tables())

        msg_id = 0
        decoded = 0
        skipped = 0

        with mcap_path.open("rb") as f:
            reader = make_reader(f, decoder_factories=[DecoderFactory()])
            for _schema, _channel, _message, ros_msg in reader.iter_decoded_messages(topics=[topic]):
                msg_dict = ros_msg_to_dict(ros_msg)
                if not isinstance(msg_dict, dict):
                    skipped += 1
                    continue
                msg_id += 1
                ctx = IngestContext(msg_id=msg_id, time_us=_stamp_from_msg(msg_dict))
                batches = adapter.flatten(msg_dict, ctx)
                for table_name, rows in batches.items():
                    if not rows:
                        continue
                    cols = list(rows[0].keys())
                    placeholders = ", ".join(["?"] * len(cols))
                    col_sql = ", ".join(cols)
                    conn.executemany(
                        f"INSERT INTO {table_name} ({col_sql}) VALUES ({placeholders})",
                        [tuple(row.get(c) for c in cols) for row in rows],
                    )
                decoded += 1

        return {
            "messages_decoded": decoded,
            "messages_skipped": skipped,
            "msg_type": msg_type,
            "topic": topic,
        }
    finally:
        conn.close()


def _stamp_from_msg(msg: dict[str, Any]) -> int:
    header = msg.get("header") or {}
    stamp = header.get("stamp") or {}
    sec = int(stamp.get("sec", 0))
    nanosec = int(stamp.get("nanosec", 0))
    return sec * 1_000_000 + nanosec // 1000
