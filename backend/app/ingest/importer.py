from __future__ import annotations

from pathlib import Path
from typing import Any, Protocol, runtime_checkable


@runtime_checkable
class FormatImporter(Protocol):
    """格式导入器：把某种原始监控数据格式（MCAP / Protobuf / CTF…）导入 DuckDB。

    这是「格式级」抽象（对应 Perfetto 的 ChunkedTraceReader）。每种格式实现一个
    Importer 并注册到 ``format_registry``，API 层据此分派，无需再硬编码 plugin_type。
    格式内部的「消息级」适配（如 ROS msg → 行）由各 Importer 自行维护二级注册表。
    """

    # 格式标识，须与数据源的 plugin_type 一致，如 "ros2_mcap"
    format_type: str

    def required_settings(self) -> list[str]:
        """ingest 所需的 settings 键列表；缺失时数据源置为 pending。"""
        ...

    def inspect(self, path: str) -> dict[str, Any]:
        """扫描原始文件，返回可供配置的结构信息（如 topic/channel 列表）。"""
        ...

    def ingest(self, db_path: Path, settings: dict[str, Any]) -> dict[str, Any]:
        """把数据导入 ``db_path`` 指向的 DuckDB，返回导入摘要（含 messages_decoded 等）。"""
        ...


class FormatImporterRegistry:
    def __init__(self) -> None:
        self._importers: dict[str, FormatImporter] = {}

    def register(self, importer: FormatImporter) -> None:
        self._importers[importer.format_type] = importer

    def get(self, format_type: str) -> FormatImporter:
        if format_type not in self._importers:
            raise KeyError(f"no importer for format: {format_type}")
        return self._importers[format_type]

    def has(self, format_type: str) -> bool:
        return format_type in self._importers

    def list_formats(self) -> list[str]:
        return list(self._importers.keys())


format_registry = FormatImporterRegistry()
