"""CTF Trace 格式导入器（``FormatImporter`` 参考实现之一）。

架构位置
--------
位于 **格式级** 扩展点（``ingest/importer.py`` 的 ``FormatImporter`` 协议），与
``McapImporter``、``ProtobufImporter`` 并列注册在 ``format_registry``。

调用链::

    前端建源 / POST ingest
      → main._try_ingest() 按 plugin_type 查 format_registry
      → CtfImporter.inspect() / ingest()
      → ctf_format 解析 trace 目录
      → DuckDB 表 + write_table_meta()
      → GET schema / POST sql/query → 前端 Timeline 泳道视图

与 MCAP 的区别：MCAP 在 Importer 之下还有 **消息级** ``RosMsgAdapter`` 二级注册表；
CTF 在 Importer 内直接完成「事件流 → span 表」映射，无 RosMsgAdapter。

新增 FormatImporter 步骤
------------------------
1. 在 ``ingest/`` 下实现格式解析模块（如 ``ctf_format.py``），或在 Importer 内联解码
2. 在本目录新建 ``your_importer.py``，实现 ``FormatImporter`` 四个接口（见下方 ``CtfImporter``）
3. 文件末尾 ``format_registry.register(YourImporter())``
4. 在 ``importers/__init__.py`` 增加 import；在 ``store.PLUGIN_META`` 声明 plugin 与 capabilities
5. （可选）前端 ``plugins/your_type/`` 建源表单 + QueryPanel

时序类 trace 须设 ``table_kind="span"`` 且含 ``_time`` + ``_dur``，前端才会启用 Timeline。
详见 ``docs/add_new_format.md``。
"""
from __future__ import annotations

from pathlib import Path
from typing import Any

import duckdb

from app.ingest import ctf_format, lttng_ctf
from app.ingest.importer import format_registry
from app.ingest.registry import ColumnDef, TableDef
from app.ingest.table_meta import write_table_meta

# 与数据源 plugin_type、PLUGIN_META 键、前端 PLUGIN_TYPE 一致
FORMAT_TYPE = "ctf"
SPAN_TABLE = "ctf_spans"


def _tables() -> list[TableDef]:
    """声明本格式写入 DuckDB 的表结构与可视化元数据。

    ingest 时用于 CREATE TABLE 并 ``write_table_meta``；Schema API / sqlBuilder 读取
    ``table_kind``、``dimension_keys``、``default_metrics`` 驱动预设 SQL 与默认图表。

    span 类表约定：``_time`` = 区间起点，``_dur`` = 区间时长；``dimension_keys[0]`` 作泳道 track。
    """
    return [
        TableDef(
            name=SPAN_TABLE,
            columns=[
                ColumnDef("span_id", "BIGINT"),
                ColumnDef("_time", "BIGINT"),
                ColumnDef("_dur", "BIGINT"),
                ColumnDef("track", "VARCHAR"),
                ColumnDef("name", "VARCHAR"),
                ColumnDef("cpu_id", "UINTEGER"),
            ],
            table_kind="span",
            dimension_keys=["track"],
            default_metrics=["_dur"],
        ),
    ]


def _build_spans(trace_dir: Path) -> list[dict[str, Any]]:
    """从 CTF trace 目录解析事件流，配对 start/end 生成 span 行（Importer 内部逻辑）。

    按 tid 维护栈：``callback_start`` 入栈，``callback_end`` 出栈配对；
    时间归一到 trace 最小起点（纳秒），避免 DuckDB 序列化时误当作 epoch 微秒。

    新 Importer：可将类似「原始记录 → 中间行 dict」的逻辑放在模块级函数或 Importer 私有方法中，
    ``ingest()`` 只负责建表、写 meta、批量 INSERT。
    """
    pending: dict[int, list[dict]] = {}
    spans: list[dict[str, Any]] = []
    for ev in ctf_format.iter_events(trace_dir):
        if ev["id"] == ctf_format.EV_CALLBACK_START:
            pending.setdefault(ev["tid"], []).append(ev)
        else:
            stack = pending.get(ev["tid"])
            if not stack:
                continue  # 没有匹配 start 的 end，跳过
            start = stack.pop()
            spans.append(
                {
                    "_time": start["ts"],
                    "_dur": ev["ts"] - start["ts"],
                    "track": f"thread-{start['tid']}",
                    "name": start.get("name", ""),
                    "cpu_id": start.get("cpu_id", 0),
                }
            )

    if not spans:
        return spans
    base = min(s["_time"] for s in spans)
    for i, s in enumerate(sorted(spans, key=lambda x: x["_time"])):
        s["span_id"] = i
        s["_time"] -= base  # 相对 trace 起点的纳秒，避免被当作 epoch 时间转换
    return spans


def _resolve_spans(trace_dir: Path) -> tuple[list[dict[str, Any]], str]:
    """根据目录形态选择解析器，返回 (span 行列表, 模式)。

    - 真实 LTTng CTF（``ros2 trace`` 产出）且系统装有 bt2 → ``lttng_ctf``（模式 "lttng"）
    - 否则 → 内置最小 CTF（模式 "builtin"）

    若识别为 LTTng 但 bt2 缺失，抛出明确错误提示安装。
    """
    if lttng_ctf.looks_like_lttng(trace_dir):
        if not lttng_ctf.is_available():
            raise RuntimeError(
                "检测到 LTTng CTF trace，但未安装 bt2（babeltrace2 Python 绑定）。"
                "请安装：apt install python3-bt2 babeltrace2，并确保 venv 可见（--system-site-packages）。"
            )
        return lttng_ctf.build_spans(trace_dir), "lttng"
    return _build_spans(trace_dir), "builtin"


class CtfImporter:
    """``FormatImporter`` 协议实现：CTF trace 目录 → span 表 → Timeline 可视化。

    支持两种 CTF 来源（由 ``_resolve_spans`` 自动识别）：
    - **真实 LTTng / ros2_tracing**（``ros2 trace`` 产出）：经 ``lttng_ctf``（bt2）解析
    - **内置最小 CTF**（``ctf_format`` 生成的测试样本）：无需任何外部依赖

    两者都映射为同一套 ``ctf_spans`` 表（``table_kind=span``），前端 Timeline 零差异。
    实现 ``FormatImporter`` 须提供的四个成员/方法见 ``ingest/importer.py``。
    """

    # registry 与 store.PLUGIN_META、数据源 plugin_type 三者须一致
    format_type = FORMAT_TYPE

    def required_settings(self) -> list[str]:
        """返回 ingest 必需的 settings 键；缺失时 API 将数据源置为 pending，不执行导入。

        前端 Form 字段名与此一致（如 ``settings['ctf.path']``）。
        """
        return ["ctf.path"]

    def inspect(self, path: str) -> dict[str, Any]:
        """扫描原始文件/目录，供建源表单「扫描」按钮使用。

        Args:
            path: CTF trace 目录（含 metadata 与 stream_*）。

        Returns:
            统一结构 ``{"topics": [{name, msg_type, message_count}, ...]}``。
            此处用 track（泳道）模拟 topic，便于前端 ``inspectSource()`` 复用 MCAP 表单逻辑。

        Raises:
            FileNotFoundError: 目录不存在。

        新 Importer：尽量返回相同 topics 结构；若无多选项，可返回单条或空列表。
        """
        trace_dir = Path(path)
        if not trace_dir.exists():
            raise FileNotFoundError(f"ctf trace not found: {path}")
        spans, _mode = _resolve_spans(trace_dir)
        tracks = sorted({s["track"] for s in spans})
        return {
            "topics": [
                {"name": t, "msg_type": t, "message_count": sum(1 for s in spans if s["track"] == t)}
                for t in tracks
            ]
        }

    def ingest(self, db_path: Path, settings: dict[str, Any]) -> dict[str, Any]:
        """解码原始数据并写入 ``db_path`` 指向的 DuckDB。

        Args:
            db_path: 该数据源专属 DuckDB 文件（由 store.duckdb_path 提供）。
            settings: 含 ``ctf.path`` 等 ``required_settings()`` 声明的键。

        Returns:
            导入摘要 dict，建议含 ``messages_decoded``（与其它 Importer 统一）；
            可附加格式专属字段（如 ``spans``）。

        标准步骤（新 Importer 可照抄）：
            1. 解析原始输入 → 行 dict 列表
            2. CREATE TABLE + DELETE（按 ``_tables()``）
            3. ``write_table_meta(conn, tables)``  # 必做
            4. executemany INSERT
        """
        trace_dir = Path(settings["ctf.path"])
        if not trace_dir.exists():
            raise FileNotFoundError(f"ctf trace not found: {trace_dir}")

        spans, mode = _resolve_spans(trace_dir)
        tables = _tables()
        conn = duckdb.connect(str(db_path))
        try:
            for t in tables:
                cols = ", ".join(f"{c.name} {c.duckdb_type}" for c in t.columns)
                conn.execute(f"CREATE TABLE IF NOT EXISTS {t.name} ({cols})")
                conn.execute(f"DELETE FROM {t.name}")
            write_table_meta(conn, tables)

            if spans:
                cols = ["span_id", "_time", "_dur", "track", "name", "cpu_id"]
                placeholders = ", ".join(["?"] * len(cols))
                conn.executemany(
                    f"INSERT INTO {SPAN_TABLE} ({', '.join(cols)}) VALUES ({placeholders})",
                    [tuple(s[c] for c in cols) for s in spans],
                )
            return {"messages_decoded": len(spans), "spans": len(spans), "ctf_mode": mode}
        finally:
            conn.close()


# 模块 import 时注册；并在 importers/__init__.py 中 import 本模块以触发注册
format_registry.register(CtfImporter())
