import app.ingest.importers  # noqa: F401 — ensure importers registered
from app.duckdb_engine import get_schema, run_query
from app.ingest import ctf_format
from app.ingest.importer import format_registry


def _write_sample_trace(trace_dir, per_thread=5):
    threads = [(1001, 0, "TimerCallback", 10_000_000, 3_000_000),
               (1002, 1, "LaserSub", 20_000_000, 6_000_000)]
    events = []
    for tid, cpu, name, period, dur in threads:
        for i in range(per_thread):
            start = i * period
            end = start + dur
            events.append((start, ctf_format.pack_callback_start(start, tid, cpu, name)))
            events.append((end, ctf_format.pack_callback_end(end, tid, cpu)))
    events.sort(key=lambda e: e[0])
    ctf_format.write_trace(trace_dir, b"".join(e for _, e in events))
    return len(threads) * per_thread


def test_ctf_importer_registered():
    assert format_registry.has("ctf")
    assert "ctf.path" in format_registry.get("ctf").required_settings()


def test_ctf_roundtrip_format(tmp_path):
    trace = tmp_path / "t"
    _write_sample_trace(trace, per_thread=3)
    evs = list(ctf_format.iter_events(trace))
    assert len(evs) == 2 * 2 * 3  # 2 线程 × 3 回调 × (start+end)
    assert evs[0]["id"] == ctf_format.EV_CALLBACK_START


def test_ctf_ingest_produces_spans(tmp_path):
    trace = tmp_path / "trace"
    n_spans = _write_sample_trace(trace, per_thread=5)
    db = tmp_path / "out.duckdb"

    report = format_registry.get("ctf").ingest(db, {"ctf.path": str(trace)})
    assert report["spans"] == n_spans

    schema = get_schema(db)
    spans_tbl = next(t for t in schema["tables"] if t["name"] == "ctf_spans")
    assert spans_tbl["table_kind"] == "span"
    assert spans_tbl["dimension_keys"] == ["track"]

    result = run_query(db, "SELECT _time, _dur, track, name FROM ctf_spans ORDER BY _time")
    assert result["meta"]["time_column"] == "_time"
    assert result["meta"]["dur_column"] == "_dur"
    assert "track" in result["meta"]["dimension_columns"]
    assert result["meta"]["row_count"] == n_spans
    # 所有区间为正、起点归一到 0
    durs = [row[1] for row in result["rows"]]
    assert all(d > 0 for d in durs)
    assert min(row[0] for row in result["rows"]) == 0
