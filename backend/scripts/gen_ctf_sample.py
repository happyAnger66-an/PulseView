"""生成模拟 ROS2 tracing 的 CTF 测试 trace（callback_start / callback_end 事件流）。

用法：
    python scripts/gen_ctf_sample.py [输出目录] [每线程回调数]
默认输出 ../test2/ctf_sample/，4 个线程、每线程 30 个回调。
"""
from __future__ import annotations

import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))

from app.ingest import ctf_format  # noqa: E402

DEFAULT_OUT = Path(__file__).resolve().parents[3] / "test2" / "ctf_sample"

# (tid, cpu_id, 回调名, 周期 ns, 区间 ns)
THREADS = [
    (1001, 0, "TimerCallback", 10_000_000, 3_500_000),
    (1002, 1, "LaserScanSub", 25_000_000, 8_000_000),
    (1003, 1, "OdomSub", 20_000_000, 4_500_000),
    (1004, 2, "ControlLoop", 50_000_000, 12_000_000),
]


def generate(out_dir: Path, per_thread: int) -> None:
    events: list[tuple[int, bytes]] = []  # (ts, encoded) 便于按时间排序后写入
    for tid, cpu, name, period, dur in THREADS:
        for i in range(per_thread):
            start = i * period + (tid % 7) * 1_000_000  # 轻微错峰
            jitter = (i * 131) % 2_000_000
            end = start + dur + jitter
            events.append((start, ctf_format.pack_callback_start(start, tid, cpu, name)))
            events.append((end, ctf_format.pack_callback_end(end, tid, cpu)))

    events.sort(key=lambda e: e[0])
    body = b"".join(enc for _, enc in events)
    ctf_format.write_trace(out_dir, body)
    spans = sum(1 for _ in THREADS) * per_thread
    print(f"wrote CTF trace ({len(events)} events / {spans} spans) -> {out_dir}")


if __name__ == "__main__":
    out = Path(sys.argv[1]) if len(sys.argv) > 1 else DEFAULT_OUT
    n = int(sys.argv[2]) if len(sys.argv) > 2 else 30
    generate(out, n)
