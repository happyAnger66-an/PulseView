# Perfetto Track Event 示例

最小化 in-process tracing 示例，演示 `TRACE_EVENT` / `TRACE_COUNTER` 并输出 `.pftrace` 文件。

## 依赖

- CMake >= 3.13
- C++17 编译器
- Perfetto amalgamated SDK（`sdk/perfetto.h` + `sdk/perfetto.cc`）

## 构建与运行

```bash
cd PulseView/examples/perfetto

# 若缺少 SDK
./fetch_sdk.sh

cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/pulseview_perfetto_example
# 可选: ./build/pulseview_perfetto_example /tmp/my_trace.pftrace
```

默认生成 `pulseview_example.pftrace`，可在 [ui.perfetto.dev](https://ui.perfetto.dev) 打开。

## 代码结构

| 文件 | 说明 |
|------|------|
| `main.cc` | 初始化、启停 session、模拟埋点、写文件 |
| `trace_categories.h` | `PERFETTO_DEFINE_CATEGORIES` |
| `trace_categories.cc` | `PERFETTO_TRACK_EVENT_STATIC_STORAGE` |

原理说明见 `PulseView/docs/tracers/perfetto.md`。
