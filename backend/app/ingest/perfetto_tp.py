"""用 Perfetto Trace Processor 解析 trace，导出 slice 区间。

为什么用 Trace Processor
------------------------
Perfetto trace 不是单一二进制布局，而是「多格式（protobuf .pftrace / Chrome JSON /
systrace / perf.data …）→ Trace Processor 归一化为 SQLite 表 → SQL 查询」的体系。
自行解析 protobuf 成本极高，正确做法是复用 Trace Processor，只消费它产出的
``slice`` / ``track`` / ``thread`` / ``process`` 表。详见 ``docs/support_perfetto.md``。

依赖
----
- Python 包 ``perfetto``（``pip install perfetto``）
- 二进制 ``trace_processor_shell``：``perfetto`` 库默认联网下载；离线环境可
  ``apt``/手动安装后用环境变量 ``PULSEVIEW_TP_SHELL`` 或 PATH 指定。
未满足时 ``is_available()`` 返回 False，``PerfettoImporter`` 给出明确安装提示。

与 lttng_ctf 的角色对应
-----------------------
``lttng_ctf`` 之于 CTF = ``perfetto_tp`` 之于 Perfetto：都是「真实 trace 的外部解析引擎」，
缺依赖时各自的 Importer 报错或回退。
"""
from __future__ import annotations

import os
import shutil
from contextlib import contextmanager
from pathlib import Path
from typing import Any, Iterator

# 取 slice → span 行的归一化查询：
# - track 名优先用「进程/线程」，回退到 track.name，再回退到 "track {id}"
# - 仅取 dur > 0 的区间（排除 instant 与未结束的 dur=-1）
SLICE_SQL = """
SELECT
  s.id AS span_id,
  s.ts AS ts,
  s.dur AS dur,
  s.name AS name,
  s.depth AS depth,
  COALESCE(s.category, '') AS category,
  COALESCE(
    (SELECT p.name || '/' || th.name
       FROM thread_track tt JOIN thread th USING(utid)
       LEFT JOIN process p USING(upid)
      WHERE tt.id = s.track_id),
    (SELECT p.name
       FROM process_track pt JOIN process p USING(upid)
      WHERE pt.id = s.track_id),
    t.name,
    'track ' || CAST(s.track_id AS TEXT)
  ) AS track
FROM slice s
JOIN track t ON s.track_id = t.id
WHERE s.dur > 0
ORDER BY s.ts
"""


def shell_path() -> str | None:
    """定位 ``trace_processor_shell`` 二进制：环境变量 > PATH；找不到返回 None。"""
    env = os.environ.get("PULSEVIEW_TP_SHELL")
    if env and Path(env).is_file():
        return env
    return shutil.which("trace_processor_shell")


def is_available() -> bool:
    """``perfetto`` 库与 ``trace_processor_shell`` 是否都就绪。"""
    try:
        import perfetto.trace_processor  # noqa: F401
    except ImportError:
        return False
    return shell_path() is not None


@contextmanager
def _open(path: Path) -> Iterator[Any]:
    """打开 trace，产出 TraceProcessor 实例（自动 close）。

    Raises:
        RuntimeError: perfetto 库或二进制缺失。
    """
    try:
        from perfetto.trace_processor import TraceProcessor, TraceProcessorConfig
    except ImportError as e:
        raise RuntimeError(
            "未安装 perfetto Python 库；请 `pip install perfetto`。"
        ) from e

    binary = shell_path()
    if binary is None:
        raise RuntimeError(
            "未找到 trace_processor_shell；请安装后用环境变量 PULSEVIEW_TP_SHELL 指定，"
            "或确保它在 PATH 中。"
        )
    tp = TraceProcessor(trace=str(path), config=TraceProcessorConfig(bin_path=binary))
    try:
        yield tp
    finally:
        tp.close()


def build_spans(path: Path) -> list[dict[str, Any]]:
    """解析 Perfetto trace，返回 span 行列表（时间归一到 trace 起点的纳秒）。

    返回行的键与 ``perfetto_importer`` 的 span 表列一致：
    ``span_id / _time / _dur / track / name / depth / category``。
    """
    spans: list[dict[str, Any]] = []
    with _open(path) as tp:
        for r in tp.query(SLICE_SQL):
            spans.append(
                {
                    "span_id": int(r.span_id),
                    "_time": int(r.ts),
                    "_dur": int(r.dur),
                    "track": r.track or "",
                    "name": r.name or "",
                    "depth": int(r.depth),
                    "category": r.category or "",
                }
            )

    if not spans:
        return spans
    base = min(s["_time"] for s in spans)
    for s in spans:
        s["_time"] -= base  # 相对 trace 起点的纳秒，避免被当作 epoch 时间转换
    return spans
