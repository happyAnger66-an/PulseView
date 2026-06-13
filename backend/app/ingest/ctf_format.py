"""最小化、自包含的 CTF（Common Trace Format 1.8）读写实现。

环境通常没有 babeltrace2 / lttng / barectf，因此这里用纯 Python 生成与解析一个
结构上符合 CTF 的 trace：一个目录，内含 ``metadata``（TSDL 文本）与一个二进制 stream。

为保持读写自洽且简单，所有整型按 little-endian、字节对齐（align = 8 bit）布局，
事件之间无填充；字符串以 NUL 结尾。生产环境若需解析任意 CTF，应改用 babeltrace2，
此处布局与下方 ``METADATA`` 中的 TSDL 完全对应。

事件模型（模拟 ROS2 tracing 的回调区间）：
    callback_start { tid, cpu_id, name }   →  压栈
    callback_end   { tid, cpu_id }         →  与同 tid 的 start 配对成一个 span
"""
from __future__ import annotations

import struct
from pathlib import Path
from typing import Iterator

MAGIC = 0xC1FC1FC1
STREAM_ID = 0
CLOCK_FREQ_HZ = 1_000_000_000  # 1 ns 分辨率

EV_CALLBACK_START = 0
EV_CALLBACK_END = 1

STREAM_FILENAME = "stream_0"
METADATA_FILENAME = "metadata"

# 内置最小 CTF 的专属标记：用于与真实 LTTng CTF 区分（两者共用 CTF magic 0xC1FC1FC1，
# 无法靠 magic/文件名判别）。写入 metadata 注释，lttng_ctf.looks_like_lttng 据此识别。
MARKER = "PULSEVIEW_MINIMAL_CTF"

# 与下方二进制布局一一对应的 TSDL 元数据（CTF 1.8，纯文本形式）
METADATA = """/* CTF 1.8 */
/* PULSEVIEW_MINIMAL_CTF */

typealias integer { size = 32; align = 8; signed = false; byte_order = le; } := uint32_t;
typealias integer { size = 64; align = 8; signed = false; byte_order = le; } := uint64_t;

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
        uint64_t timestamp;
    };
};

event {
    id = 0;
    name = "ros2:callback_start";
    stream_id = 0;
    fields := struct {
        uint32_t tid;
        uint32_t cpu_id;
        string name;
    };
};

event {
    id = 1;
    name = "ros2:callback_end";
    stream_id = 0;
    fields := struct {
        uint32_t tid;
        uint32_t cpu_id;
    };
};
"""

_U32 = struct.Struct("<I")
_U64 = struct.Struct("<Q")
_HDR = struct.Struct("<IQ")  # event header: id(u32) + timestamp(u64)


def _pack_string(s: str) -> bytes:
    return s.encode("utf-8") + b"\x00"


def pack_packet_header() -> bytes:
    return _U32.pack(MAGIC) + _U32.pack(STREAM_ID)


def pack_callback_start(ts: int, tid: int, cpu_id: int, name: str) -> bytes:
    return (
        _HDR.pack(EV_CALLBACK_START, ts)
        + _U32.pack(tid)
        + _U32.pack(cpu_id)
        + _pack_string(name)
    )


def pack_callback_end(ts: int, tid: int, cpu_id: int) -> bytes:
    return _HDR.pack(EV_CALLBACK_END, ts) + _U32.pack(tid) + _U32.pack(cpu_id)


def write_trace(trace_dir: Path, packet_body: bytes) -> None:
    """写出一个单 packet 的 CTF trace 目录（metadata + stream_0）。"""
    trace_dir.mkdir(parents=True, exist_ok=True)
    (trace_dir / METADATA_FILENAME).write_text(METADATA, encoding="utf-8")
    (trace_dir / STREAM_FILENAME).write_bytes(pack_packet_header() + packet_body)


def _read_string(buf: bytes, pos: int) -> tuple[str, int]:
    end = buf.index(b"\x00", pos)
    return buf[pos:end].decode("utf-8"), end + 1


def iter_events(trace_dir: Path) -> Iterator[dict]:
    """解析 CTF stream，逐条产出事件 dict。

    校验 packet.header.magic，随后按 event.header + 各事件字段顺序读取直到 EOF。
    """
    data = (trace_dir / STREAM_FILENAME).read_bytes()
    pos = 0
    magic, pos = _U32.unpack_from(data, pos)[0], pos + 4
    if magic != MAGIC:
        raise ValueError(f"bad CTF magic: {magic:#x}")
    _stream_id, pos = _U32.unpack_from(data, pos)[0], pos + 4

    n = len(data)
    while pos < n:
        ev_id, ts = _HDR.unpack_from(data, pos)
        pos += _HDR.size
        if ev_id == EV_CALLBACK_START:
            tid = _U32.unpack_from(data, pos)[0]; pos += 4
            cpu_id = _U32.unpack_from(data, pos)[0]; pos += 4
            name, pos = _read_string(data, pos)
            yield {"id": ev_id, "ts": ts, "tid": tid, "cpu_id": cpu_id, "name": name}
        elif ev_id == EV_CALLBACK_END:
            tid = _U32.unpack_from(data, pos)[0]; pos += 4
            cpu_id = _U32.unpack_from(data, pos)[0]; pos += 4
            yield {"id": ev_id, "ts": ts, "tid": tid, "cpu_id": cpu_id}
        else:
            raise ValueError(f"unknown event id {ev_id} at offset {pos}")
