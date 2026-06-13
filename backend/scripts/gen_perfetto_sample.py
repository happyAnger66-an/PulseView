"""生成 Perfetto 可解析的测试 trace（Chrome JSON trace 格式）。

为什么用 Chrome JSON：原生 ``.perfetto-trace``（protobuf）生成需 protobuf 6.x，
与本项目运行时（5.x）冲突；而 Trace Processor 把 Chrome JSON 归一化为同一套
``slice`` / ``track`` 表，PerfettoImporter 解析路径完全一致，是零依赖的理想夹具。
``ph:"X"`` = 完整区间事件（带 dur）；``ph:"M"`` = 进程/线程命名元数据。
时间单位为微秒（Trace Processor 会转为纳秒）。

用法：
    python scripts/gen_perfetto_sample.py [输出文件] [每线程区间数]
默认输出 ../test2/perfetto_sample.json，3 个线程、每线程 20 个顶层区间（含嵌套子区间）。
"""
from __future__ import annotations

import json
import sys
from pathlib import Path

DEFAULT_OUT = Path(__file__).resolve().parents[3] / "test2" / "perfetto_sample.json"

# (pid, tid, 进程名, 线程名, 顶层区间名, 子区间名, 周期 us, 区间 us)
THREADS = [
    (100, 10, "perception", "main", "LaserCallback", "process_scan", 10_000, 3_500),
    (100, 11, "perception", "worker", "OdomCallback", "integrate", 8_000, 2_000),
    (200, 20, "planning", "main", "PlanLoop", "optimize", 20_000, 9_000),
]


def generate(out_path: Path, per_thread: int) -> None:
    events: list[dict] = []
    for pid, tid, pname, tname, top, child, period, dur in THREADS:
        events.append({"name": "process_name", "ph": "M", "pid": pid, "tid": tid,
                       "args": {"name": pname}})
        events.append({"name": "thread_name", "ph": "M", "pid": pid, "tid": tid,
                       "args": {"name": tname}})
        for i in range(per_thread):
            start = i * period + (tid % 5) * 500
            jitter = (i * 137) % 1_500
            d = dur + jitter
            events.append({"name": top, "ph": "X", "ts": start, "dur": d,
                           "pid": pid, "tid": tid, "cat": "ros"})
            # 嵌套子区间（depth=1），位于父区间内部
            events.append({"name": child, "ph": "X", "ts": start + d // 4,
                           "dur": d // 2, "pid": pid, "tid": tid, "cat": "ros"})

    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_text(json.dumps(events), encoding="utf-8")
    spans = len(THREADS) * per_thread * 2
    print(f"wrote Perfetto (Chrome JSON) trace ({len(events)} events / ~{spans} slices) -> {out_path}")


if __name__ == "__main__":
    out = Path(sys.argv[1]) if len(sys.argv) > 1 else DEFAULT_OUT
    n = int(sys.argv[2]) if len(sys.argv) > 2 else 20
    generate(out, n)
