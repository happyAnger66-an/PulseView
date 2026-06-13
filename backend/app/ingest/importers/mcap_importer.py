from __future__ import annotations

from pathlib import Path
from typing import Any

from app.ingest.importer import format_registry
from app.ingest.pipeline import ingest_mcap
from app.ingest.mcap_inspect import inspect_mcap

FORMAT_TYPE = "ros2_mcap"


class McapImporter:
    """ROS2 MCAP 格式导入器。

    薄封装现有 MCAP 管线：``inspect`` 列出 topic，``ingest`` 经 RosMsgAdapter 二级注册表
    解码消息并写入 DuckDB。消息级扩展仍通过 ``app.ingest.adapters`` 的 registry 完成。
    """

    format_type = FORMAT_TYPE

    def required_settings(self) -> list[str]:
        return ["mcap.path", "mcap.topic", "mcap.msg_type"]

    def inspect(self, path: str) -> dict[str, Any]:
        return inspect_mcap(path)

    def ingest(self, db_path: Path, settings: dict[str, Any]) -> dict[str, Any]:
        return ingest_mcap(
            db_path,
            Path(settings["mcap.path"]),
            settings["mcap.topic"],
            settings["mcap.msg_type"],
        )


format_registry.register(McapImporter())
