# 支持真实 LTTng / ros2_tracing CTF

PulseView 的 CTF 数据源同时支持两种来源，由 `CtfImporter` 自动识别、零配置分派：

| 来源 | 解析器 | 依赖 | 说明 |
| --- | --- | --- | --- |
| 内置最小 CTF | `app/ingest/ctf_format.py` | 无 | 测试样本（`scripts/gen_ctf_sample.py`），metadata 带 `PULSEVIEW_MINIMAL_CTF` 标记 |
| 真实 LTTng / `ros2 trace` | `app/ingest/lttng_ctf.py` | 系统 `bt2`（babeltrace2 Python 绑定） | 解析 `ros2:callback_*` 事件，配对成 span |

两者最终都写入同一张 `ctf_spans` 表（`table_kind=span`），前端 Timeline 视图无差异。

## 一、让 PulseView 能读真实 CTF（目标 A）

真实 LTTng CTF 用 [babeltrace2](https://babeltrace.org/) 解析，其 Python 绑定 `bt2`
**无法用 pip 安装**，只能装系统包，再让虚拟环境可见。

```bash
# 1. 安装系统包
sudo apt install babeltrace2 python3-bt2

# 2. 重建虚拟环境，带上 system site-packages（方式 1）
cd PulseView/backend
rm -rf .venv
python3 -m venv --system-site-packages .venv
.venv/bin/pip install -r requirements.txt -r requirements-dev.txt

# 3. 验证
.venv/bin/python -c "import bt2; print(bt2.__version__)"
```

> 备选「方式 2」：保留现有 venv，在其 `site-packages` 下放一个 `.pth` 文件指向
> 系统的 `dist-packages`。方式 1 更简单可靠，推荐。

装好后，CTF 数据源的 `ctf.path` 指向 LTTng trace 目录即可（支持嵌套的
`session/ust/uid/<uid>/<bitness>/` 结构，`bt2` 会自动发现）。

## 二、生成真实 ros2_tracing trace（目标 B，需要 ROS 2）

```bash
sudo apt install lttng-tools liblttng-ust-dev
# 安装 ROS 2 与 ros2_tracing 后：
ros2 trace start my_session            # 启动会话（默认采集 ros2:* UST 事件）
# 另一终端运行你的 ROS 2 节点……
ros2 trace stop                        # 停止，trace 落到 ~/.ros/tracing/my_session
```

将该目录作为 CTF 数据源的 `ctf.path` 导入即可。

## 解析细节

`lttng_ctf.build_spans` 使用 `bt2.TraceCollectionMessageIterator` 遍历事件：

- `ros2:rclcpp_callback_register {callback, symbol}` → 建立「回调指针 → 函数符号」映射
- `ros2:callback_start {callback, ...}` + context `vtid/procname` → 入栈
- `ros2:callback_end {callback}` → 出栈配对成 span

每个 span：`_time`（归一到 trace 起点的纳秒）、`_dur`、`track = procname-vtid`、
`name = 符号`、`cpu_id = vtid`。栈键为 `(vtid, callback)`，支持同线程递归/重入。

缺少 `bt2` 时：若目录被识别为真实 LTTng（metadata 无内置标记），`ingest` 会抛出
明确错误提示安装；内置最小 CTF 与其它格式不受影响。
