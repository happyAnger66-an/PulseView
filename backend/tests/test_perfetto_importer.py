"""Perfetto 导入器测试（slice → span）。

用 Chrome JSON trace 作夹具（Trace Processor 归一化为同一套 slice/track 表，
与原生 .perfetto-trace 解析路径一致）。缺 trace_processor_shell 时整模块 skip。
"""
from __future__ import annotations

import json

import pytest

import app.ingest.importers  # noqa: F401 — ensure importers registered
from app.duckdb_engine import get_schema, run_query
from app.ingest import perfetto_tp
from app.ingest.importer import format_registry

pytestmark = pytest.mark.skipif(
    not perfetto_tp.is_available(), reason="trace_processor_shell not available"
)


def _write_trace(path, per_thread=4):
    threads = [
        (100, 10, "perception", "main", "LaserCb", "scan", 10_000, 3_000),
        (200, 20, "planning", "worker", "PlanLoop", "opt", 15_000, 5_000),
    ]
    events = []
    for pid, tid, pname, tname, top, child, period, dur in threads:
        events.append({"name": "process_name", "ph": "M", "pid": pid, "tid": tid,
                       "args": {"name": pname}})
        events.append({"name": "thread_name", "ph": "M", "pid": pid, "tid": tid,
                       "args": {"name": tname}})
        for i in range(per_thread):
            start = i * period
            events.append({"name": top, "ph": "X", "ts": start, "dur": dur,
                           "pid": pid, "tid": tid, "cat": "ros"})
            events.append({"name": child, "ph": "X", "ts": start + dur // 4,
                           "dur": dur // 2, "pid": pid, "tid": tid, "cat": "ros"})
    path.write_text(json.dumps(events), encoding="utf-8")
    # 每线程 per_thread 个顶层 + per_thread 个子区间
    return len(threads) * per_thread * 2


def test_perfetto_importer_registered():
    assert format_registry.has("perfetto")
    assert "perfetto.path" in format_registry.get("perfetto").required_settings()


def test_build_spans_normalizes_and_resolves_tracks(tmp_path):
    trace = tmp_path / "t.json"
    expected = _write_trace(trace, per_thread=4)
    spans = perfetto_tp.build_spans(trace)

    assert len(spans) == expected
    assert min(s["_time"] for s in spans) == 0
    assert all(s["_dur"] > 0 for s in spans)
    # track = 进程/线程名
    assert {"perception/main", "planning/worker"} == {s["track"] for s in spans}
    # 嵌套子区间 depth=1 存在
    assert any(s["depth"] == 1 for s in spans)
    assert {"LaserCb", "scan", "PlanLoop", "opt"} == {s["name"] for s in spans}


def test_ingest_produces_span_table(tmp_path):
    trace = tmp_path / "trace.json"
    expected = _write_trace(trace, per_thread=5)
    db = tmp_path / "out.duckdb"

    report = format_registry.get("perfetto").ingest(db, {"perfetto.path": str(trace)})
    assert report["spans"] == expected

    schema = get_schema(db)
    spans_tbl = next(t for t in schema["tables"] if t["name"] == "perfetto_slices")
    assert spans_tbl["table_kind"] == "span"
    assert spans_tbl["dimension_keys"] == ["track"]

    result = run_query(db, "SELECT _time, _dur, track, name FROM perfetto_slices ORDER BY _time")
    assert result["meta"]["time_column"] == "_time"
    assert result["meta"]["dur_column"] == "_dur"
    assert "track" in result["meta"]["dimension_columns"]
    assert result["meta"]["row_count"] == expected


def test_inspect_returns_tracks(tmp_path):
    trace = tmp_path / "t.json"
    _write_trace(trace, per_thread=3)
    res = format_registry.get("perfetto").inspect(str(trace))
    names = {t["name"] for t in res["topics"]}
    assert names == {"perception/main", "planning/worker"}
    assert all(t["message_count"] > 0 for t in res["topics"])
