"""用 babeltrace2（``bt2``）解析真实 LTTng / ros2_tracing 产出的 CTF trace。

与 ``ctf_format.py`` 的区别
--------------------------
- ``ctf_format``：PulseView 自研的最小 CTF（单 packet、固定布局），用于无依赖的测试样本。
- 本模块：解析 **真实** LTTng-UST trace（``ros2 trace`` 的输出），依赖系统的 ``bt2``。

依赖
----
需要 Python ``bt2`` 模块（Ubuntu: ``apt install python3-bt2 babeltrace2``）。
未安装时 ``is_available()`` 返回 False，``CtfImporter`` 会回退到内置 CTF 解析。

事件模型（ros2_tracing）
------------------------
- ``ros2:rclcpp_callback_register {callback, symbol}``  —— 建立 callback 指针 → 函数符号映射
- ``ros2:callback_start {callback, is_intra_process}``  —— 区间开始（context: procname/vpid/vtid）
- ``ros2:callback_end {callback}``                      —— 区间结束

按 ``(vtid, callback)`` 用栈配对 start/end 生成 span，track = ``procname-vtid``，name = 符号。
输出行结构与 ``ctf_importer`` 的 span 表一致：``_time / _dur / track / name / cpu_id``。
"""
from __future__ import annotations

from pathlib import Path
from typing import Any

CALLBACK_START = "ros2:callback_start"
CALLBACK_END = "ros2:callback_end"
CALLBACK_REGISTER = "ros2:rclcpp_callback_register"


def is_available() -> bool:
    """bt2 是否可用（决定能否解析真实 LTTng CTF）。"""
    try:
        import bt2  # noqa: F401
    except ImportError:
        return False
    return True


def looks_like_lttng(path: Path) -> bool:
    """判断目录是否为真实 LTTng CTF（区别于内置最小 CTF）。

    CTF 标准 magic（0xC1FC1FC1）与文件名（stream_0）两者通用，无法据此区分，
    因此改用「内置格式专属标记」反向判别：metadata 含 ``ctf_format.MARKER`` → 内置；
    否则只要（含子目录）存在 metadata 文件，即按真实 LTTng 处理。
    """
    from app.ingest import ctf_format

    for meta in _iter_metadata_files(path):
        try:
            data = meta.read_bytes()
        except OSError:
            continue
        if ctf_format.MARKER.encode() in data:
            return False  # 命中内置最小 CTF
        return True  # 存在非内置标记的 metadata → 真实 LTTng
    return False


def _iter_metadata_files(path: Path):
    direct = path / "metadata"
    if direct.is_file():
        yield direct
    else:
        yield from (p for p in path.rglob("metadata") if p.is_file())


def _field(event, name: str):
    """从 event 的 payload / common_context / specific_context 中按名取字段值。"""
    for getter in ("payload_field", "common_context_field", "specific_context_field"):
        scope = getattr(event, getter, None)
        if scope is None:
            continue
        try:
            return scope[name]
        except (KeyError, TypeError):
            continue
    return None


def build_spans(path: Path) -> list[dict[str, Any]]:
    """解析 LTTng CTF，返回 span 行列表（时间归一到 trace 起点的纳秒）。

    Raises:
        RuntimeError: bt2 不可用。
    """
    import bt2

    symbol_map: dict[int, str] = {}
    # 栈键 (vtid, callback_ptr) → 开始事件信息
    pending: dict[tuple[int, int], list[dict[str, Any]]] = {}
    spans: list[dict[str, Any]] = []

    for msg in bt2.TraceCollectionMessageIterator(str(path)):
        if type(msg) is not bt2._EventMessageConst:
            continue
        event = msg.event
        name = event.name
        try:
            ts = msg.default_clock_snapshot.ns_from_origin
        except (bt2._Error, ValueError):
            continue

        if name == CALLBACK_REGISTER:
            cb = _field(event, "callback")
            sym = _field(event, "symbol")
            if cb is not None and sym is not None:
                symbol_map[int(cb)] = str(sym)
            continue

        if name == CALLBACK_START:
            cb = _field(event, "callback")
            vtid = _field(event, "vtid")
            procname = _field(event, "procname")
            if cb is None:
                continue
            key = (int(vtid) if vtid is not None else -1, int(cb))
            pending.setdefault(key, []).append(
                {
                    "ts": ts,
                    "callback": int(cb),
                    "vtid": int(vtid) if vtid is not None else 0,
                    "procname": str(procname) if procname is not None else "unknown",
                }
            )
            continue

        if name == CALLBACK_END:
            cb = _field(event, "callback")
            vtid = _field(event, "vtid")
            if cb is None:
                continue
            key = (int(vtid) if vtid is not None else -1, int(cb))
            stack = pending.get(key)
            if not stack:
                continue
            start = stack.pop()
            spans.append(
                {
                    "_time": start["ts"],
                    "_dur": ts - start["ts"],
                    "track": f"{start['procname']}-{start['vtid']}",
                    "callback": start["callback"],
                    "cpu_id": start["vtid"],
                }
            )

    if not spans:
        return spans

    base = min(s["_time"] for s in spans)
    for i, s in enumerate(sorted(spans, key=lambda x: x["_time"])):
        s["span_id"] = i
        s["_time"] -= base
        s["name"] = symbol_map.get(s.pop("callback"), "")
    return spans
