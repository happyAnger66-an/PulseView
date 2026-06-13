"""Perfetto trace 格式导入器（``FormatImporter`` 实现之一）。

架构位置
--------
位于 **格式级** 扩展点（``ingest/importer.py`` 的 ``FormatImporter`` 协议），与
``McapImporter``、``ProtobufImporter``、``CtfImporter`` 并列注册在 ``format_registry``。

调用链::

    前端建源 / POST ingest
      → main._try_ingest() 按 plugin_type 查 format_registry
      → PerfettoImporter.inspect() / ingest()
      → perfetto_tp（Trace Processor）解析 trace → slice
      → DuckDB perfetto_slices 表（table_kind=span）+ write_table_meta()
      → GET schema / POST sql/query → 前端 Timeline 泳道视图

支持的输入：凡 Trace Processor 能识别的格式——原生 ``.perfetto-trace`` / ``.pftrace``
（protobuf）、Chrome JSON trace、systrace、perf.data 等，无需在本侧区分。

阶段 1（MVP）：仅导出 slice → span。counter / flow / 嵌套 track 见
``docs/support_perfetto.md`` 后续阶段。
"""
from __future__ import annotations

from pathlib import Path
from typing import Any

import duckdb

from app.ingest import perfetto_tp
from app.ingest.importer import format_registry
from app.ingest.registry import ColumnDef, TableDef
from app.ingest.table_meta import write_table_meta

# 与数据源 plugin_type、PLUGIN_META 键、前端 PLUGIN_TYPE 一致
FORMAT_TYPE = "perfetto"
SPAN_TABLE = "perfetto_slices"
SETTING_PATH = "perfetto.path"


def _tables() -> list[TableDef]:
    """声明写入 DuckDB 的表结构与可视化元数据。

    span 类表约定：``_time`` = 区间起点，``_dur`` = 区间时长，``track`` 作泳道维度。
    ``depth`` / ``category`` 为附加列，供后续嵌套渲染或过滤使用。
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
                ColumnDef("depth", "INTEGER"),
                ColumnDef("category", "VARCHAR"),
            ],
            table_kind="span",
            dimension_keys=["track"],
            default_metrics=["_dur"],
        ),
    ]


def _require_available() -> None:
    """Trace Processor 不可用时抛出明确安装提示。"""
    if not perfetto_tp.is_available():
        raise RuntimeError(
            "Perfetto 解析依赖未就绪：需 `pip install perfetto` 且可用 trace_processor_shell"
            "（用环境变量 PULSEVIEW_TP_SHELL 指定，或放入 PATH）。详见 docs/support_perfetto.md。"
        )


class PerfettoImporter:
    """``FormatImporter`` 协议实现：Perfetto trace → slice → span 表 → Timeline。

    实现 ``FormatImporter`` 须提供的四个成员/方法见 ``ingest/importer.py``。
    """

    format_type = FORMAT_TYPE

    def required_settings(self) -> list[str]:
        """ingest 必需的 settings 键；缺失时数据源置为 pending，不执行导入。"""
        return [SETTING_PATH]

    def inspect(self, path: str) -> dict[str, Any]:
        """扫描 trace，返回泳道（track）列表，供建源表单「扫描」按钮使用。

        Args:
            path: Perfetto trace 文件路径（.perfetto-trace / .pftrace / Chrome JSON 等）。

        Returns:
            ``{"topics": [{name, msg_type, message_count}, ...]}``，用 track 模拟 topic，
            便于前端复用 MCAP 的 ``inspectSource()`` 表单逻辑。

        Raises:
            FileNotFoundError: 文件不存在。
            RuntimeError: Trace Processor 不可用。
        """
        trace = Path(path)
        if not trace.exists():
            raise FileNotFoundError(f"perfetto trace not found: {path}")
        _require_available()
        spans = perfetto_tp.build_spans(trace)
        tracks = sorted({s["track"] for s in spans})
        return {
            "topics": [
                {"name": t, "msg_type": t, "message_count": sum(1 for s in spans if s["track"] == t)}
                for t in tracks
            ]
        }

    def ingest(self, db_path: Path, settings: dict[str, Any]) -> dict[str, Any]:
        """解析 trace 并写入 ``db_path`` 指向的 DuckDB。

        Args:
            db_path: 该数据源专属 DuckDB 文件（由 store.duckdb_path 提供）。
            settings: 含 ``perfetto.path``。

        Returns:
            导入摘要，含 ``messages_decoded`` / ``spans``。

        Raises:
            FileNotFoundError: 文件不存在。
            RuntimeError: Trace Processor 不可用。
        """
        trace = Path(settings[SETTING_PATH])
        if not trace.exists():
            raise FileNotFoundError(f"perfetto trace not found: {trace}")
        _require_available()

        spans = perfetto_tp.build_spans(trace)
        tables = _tables()
        conn = duckdb.connect(str(db_path))
        try:
            for t in tables:
                cols = ", ".join(f"{c.name} {c.duckdb_type}" for c in t.columns)
                conn.execute(f"CREATE TABLE IF NOT EXISTS {t.name} ({cols})")
                conn.execute(f"DELETE FROM {t.name}")
            write_table_meta(conn, tables)

            if spans:
                cols = ["span_id", "_time", "_dur", "track", "name", "depth", "category"]
                placeholders = ", ".join(["?"] * len(cols))
                conn.executemany(
                    f"INSERT INTO {SPAN_TABLE} ({', '.join(cols)}) VALUES ({placeholders})",
                    [tuple(s[c] for c in cols) for s in spans],
                )
            return {"messages_decoded": len(spans), "spans": len(spans)}
        finally:
            conn.close()


# 模块 import 时注册；并在 importers/__init__.py 中 import 本模块以触发注册
format_registry.register(PerfettoImporter())
