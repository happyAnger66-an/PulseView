"""LTTng / ros2_tracing CTF 读取分支测试。

本机无 lttng-tools / ROS 2，无法用 ``ros2 trace`` 产生真实 trace；但 bt2 能解析
合法的 CTF 1.8。这里手写一个「LTTng 形态」CTF 夹具：带 ``ros2:callback_*`` 事件、
event.context(vtid/procname)、clock 映射——用以端到端验证 ``lttng_ctf`` 解析与
``CtfImporter`` 的分派。缺 bt2 时整模块 skip。
"""
from __future__ import annotations

import struct

import pytest

import app.ingest.importers  # noqa: F401 — ensure importers registered
from app.duckdb_engine import get_schema, run_query
from app.ingest import lttng_ctf
from app.ingest.importer import format_registry

pytestmark = pytest.mark.skipif(not lttng_ctf.is_available(), reason="bt2 not installed")

# 与下方二进制布局对应的 TSDL（plain-text CTF 1.8，bt2 可解析）
_METADATA = """/* CTF 1.8 */
typealias integer { size = 32; align = 8; signed = false; byte_order = le; } := uint32_t;
typealias integer { size = 64; align = 8; signed = false; byte_order = le; } := uint64_t;
typealias integer { size = 32; align = 8; signed = true; byte_order = le; } := int32_t;
typealias integer { size = 64; align = 8; signed = false; byte_order = le; map = clock.monotonic.value; } := uint64_clock_t;

trace {
    major = 1;
    minor = 8;
    byte_order = le;
    packet.header := struct {
        uint32_t magic;
        uint32_t stream_id;
    };
};

clock {
    name = monotonic;
    freq = 1000000000;
};

stream {
    id = 0;
    event.header := struct {
        uint32_t id;
        uint64_clock_t timestamp;
    };
    event.context := struct {
        int32_t vtid;
        string procname;
    };
};

event {
    id = 0;
    name = "ros2:rclcpp_callback_register";
    stream_id = 0;
    fields := struct {
        uint64_t callback;
        string symbol;
    };
};
event {
    id = 1;
    name = "ros2:callback_start";
    stream_id = 0;
    fields := struct {
        uint64_t callback;
        int32_t is_intra_process;
    };
};
event {
    id = 2;
    name = "ros2:callback_end";
    stream_id = 0;
    fields := struct {
        uint64_t callback;
    };
};
"""

MAGIC = 0xC1FC1FC1
_HDR = struct.Struct("<IQ")  # event.header: id(u32) + timestamp(u64)


def _cstr(s: str) -> bytes:
    return s.encode("utf-8") + b"\x00"


def _ctx(vtid: int, procname: str) -> bytes:
    return struct.pack("<i", vtid) + _cstr(procname)


def _ev_register(ts, vtid, procname, callback, symbol):
    return _HDR.pack(0, ts) + _ctx(vtid, procname) + struct.pack("<Q", callback) + _cstr(symbol)


def _ev_start(ts, vtid, procname, callback):
    return _HDR.pack(1, ts) + _ctx(vtid, procname) + struct.pack("<Q", callback) + struct.pack("<i", 0)


def _ev_end(ts, vtid, procname, callback):
    return _HDR.pack(2, ts) + _ctx(vtid, procname) + struct.pack("<Q", callback)


def _write_lttng_like_trace(trace_dir, per_callback=4):
    """写一个 bt2 可解析的 LTTng 形态 CTF。

    两个回调（指针 + 符号），各在独立线程上多次 start/end。
    """
    trace_dir.mkdir(parents=True, exist_ok=True)
    (trace_dir / "metadata").write_text(_METADATA, encoding="utf-8")

    callbacks = [
        (0xAAAA0001, 1101, "comp_node", "TimerCb()"),
        (0xBBBB0002, 1102, "comp_node", "LaserCb()"),
    ]
    body = struct.pack("<I", MAGIC) + struct.pack("<I", 0)  # packet.header
    ts = 1000
    # 先注册符号
    for cb, vtid, proc, sym in callbacks:
        body += _ev_register(ts, vtid, proc, cb, sym)
        ts += 100
    # 再产生区间
    expected = 0
    for i in range(per_callback):
        for cb, vtid, proc, _sym in callbacks:
            start = ts
            end = ts + 5_000 + i * 100
            body += _ev_start(start, vtid, proc, cb)
            body += _ev_end(end, vtid, proc, cb)
            ts = end + 1_000
            expected += 1
    (trace_dir / "stream_0").write_bytes(body)
    return expected


def test_looks_like_lttng_distinguishes_formats(tmp_path):
    # 内置最小 CTF → 不应识别为 LTTng
    from app.ingest import ctf_format

    builtin = tmp_path / "builtin"
    ctf_format.write_trace(builtin, b"")
    assert lttng_ctf.looks_like_lttng(builtin) is False

    # LTTng 形态（无 builtin magic 的 stream_0）→ 识别为 LTTng
    lt = tmp_path / "lttng"
    _write_lttng_like_trace(lt)
    assert lttng_ctf.looks_like_lttng(lt) is True


def test_build_spans_pairs_callbacks_and_resolves_symbols(tmp_path):
    trace = tmp_path / "t"
    expected = _write_lttng_like_trace(trace, per_callback=4)
    spans = lttng_ctf.build_spans(trace)

    assert len(spans) == expected
    assert min(s["_time"] for s in spans) == 0
    assert all(s["_dur"] > 0 for s in spans)
    # 符号经 rclcpp_callback_register 解析
    assert {s["name"] for s in spans} == {"TimerCb()", "LaserCb()"}
    # track 为 procname-vtid
    assert {s["track"] for s in spans} == {"comp_node-1101", "comp_node-1102"}


def test_ctf_importer_ingests_lttng_trace(tmp_path):
    trace = tmp_path / "trace"
    expected = _write_lttng_like_trace(trace, per_callback=5)
    db = tmp_path / "out.duckdb"

    report = format_registry.get("ctf").ingest(db, {"ctf.path": str(trace)})
    assert report["ctf_mode"] == "lttng"
    assert report["spans"] == expected

    schema = get_schema(db)
    spans_tbl = next(t for t in schema["tables"] if t["name"] == "ctf_spans")
    assert spans_tbl["table_kind"] == "span"

    result = run_query(db, "SELECT _time, _dur, track, name FROM ctf_spans ORDER BY _time")
    assert result["meta"]["time_column"] == "_time"
    assert result["meta"]["dur_column"] == "_dur"
    assert "track" in result["meta"]["dimension_columns"]
    assert result["meta"]["row_count"] == expected
