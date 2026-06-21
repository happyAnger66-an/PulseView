#!/usr/bin/env python3
"""Verify a ros2 trace directory parses into PulseView span rows."""
from __future__ import annotations

import argparse
import sys
from pathlib import Path

BACKEND = Path(__file__).resolve().parents[2] / "backend"
sys.path.insert(0, str(BACKEND))

import app.ingest.importers  # noqa: F401
from app.duckdb_engine import get_schema, run_query
from app.ingest import lttng_ctf
from app.ingest.importer import format_registry


def _resolve_trace_dir(path: Path) -> Path:
    if not path.is_dir():
        raise SystemExit(f"trace directory not found: {path}")
    if lttng_ctf.looks_like_lttng(path):
        return path
    # ros2 trace layout: <base>/<session>/ust/...
    for candidate in sorted(path.rglob("metadata")):
        parent = candidate.parent
        if lttng_ctf.looks_like_lttng(parent):
            return parent
    raise SystemExit(f"no LTTng CTF metadata found under: {path}")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("trace_path", type=Path, help="ros2 trace session or ust directory")
    parser.add_argument("--min-spans", type=int, default=1, help="minimum expected spans")
    args = parser.parse_args()

    if not lttng_ctf.is_available():
        print("ERROR: bt2 not available — install python3-bt2 and rebuild venv with --system-site-packages")
        return 2

    session_dir = args.trace_path.resolve()
    trace_dir = _resolve_trace_dir(session_dir)
    spans = lttng_ctf.build_spans(session_dir)
    print(f"session   : {session_dir}")
    print(f"metadata  : {trace_dir}")
    print(f"spans     : {len(spans)}")
    if spans:
        sample = spans[0]
        print(f"sample    : track={sample.get('track')!r} name={sample.get('name')!r} dur={sample.get('_dur')}")

    if len(spans) < args.min_spans:
        print(f"FAIL: expected at least {args.min_spans} spans, got {len(spans)}")
        return 1

    importer = format_registry.get("ctf")
    db_path = Path("/tmp/pv_ros2_trace_verify.duckdb")
    if db_path.exists():
        db_path.unlink()
    report = importer.ingest(db_path, {"ctf.path": str(session_dir)})
    print(f"ingest    : mode={report.get('ctf_mode')} spans={report.get('spans')}")

    schema = get_schema(db_path)
    tables = [t["name"] for t in schema.get("tables", [])]
    if "ctf_spans" not in tables:
        print("FAIL: ctf_spans table missing after ingest")
        return 1

    result = run_query(db_path, "SELECT COUNT(*) AS n FROM ctf_spans")
    rows = result["rows"]
    count = rows[0][0] if rows else 0
    print(f"duckdb    : ctf_spans rows = {count}")
    if count < args.min_spans:
        print(f"FAIL: DuckDB has {count} rows, expected >= {args.min_spans}")
        return 1

    print("OK: ros2 trace is usable by PulseView CTF importer")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
