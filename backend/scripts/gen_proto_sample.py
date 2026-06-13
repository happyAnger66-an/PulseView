"""生成 length-delimited 的 pulseview.MetricSample protobuf 测试文件。

用法：
    python scripts/gen_proto_sample.py [输出路径] [样本条数]
默认输出 ../test2/proto_sample.pb，120 条采样、4 个 CPU 核。
"""
from __future__ import annotations

import math
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))

from app.ingest.proto_schema import encode_delimited, get_message_class  # noqa: E402

# 默认写入 PulseView 同级的 test2/ 目录（与 e2e fixture 约定一致）
DEFAULT_OUT = Path(__file__).resolve().parents[3] / "test2" / "proto_sample.pb"
NUM_CORES = 4


def generate(out_path: Path, count: int) -> None:
    MetricSample = get_message_class()
    base_us = 1_700_000_000_000_000  # 起始时间（微秒）
    step_us = 1_000_000  # 1s 间隔

    out_path.parent.mkdir(parents=True, exist_ok=True)
    with out_path.open("wb") as f:
        for i in range(count):
            msg = MetricSample()
            msg.time_us = base_us + i * step_us
            msg.host = "host-a"
            msg.cpu_percent = 50 + 40 * math.sin(i / 12)
            msg.mem_percent = 60 + 20 * math.cos(i / 18)
            for c in range(NUM_CORES):
                core = msg.cores.add()
                core.core = f"cpu{c}"
                core.usage = 40 + 30 * math.sin((i + c * 5) / 10)
            f.write(encode_delimited(msg))

    print(f"wrote {count} samples ({NUM_CORES} cores each) -> {out_path}")


if __name__ == "__main__":
    out = Path(sys.argv[1]) if len(sys.argv) > 1 else DEFAULT_OUT
    n = int(sys.argv[2]) if len(sys.argv) > 2 else 120
    generate(out, n)
