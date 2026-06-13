"""MCAP 文件扫描：列出 Topic 与消息类型，供建源表单「扫描」使用。

架构位置：MCAP 格式专属的 inspect 实现，与 ``pipeline.py`` 并列于 ``ingest/`` 包内；
被 ``McapImporter.inspect()`` 调用，API 层通过 ``GET /api/inspect?plugin_type=ros2_mcap``
间接使用，不直接暴露本模块。

返回结构与 Protobuf/CTF 的 inspect 一致：``{"topics": [{name, msg_type, message_count}]}``，
便于前端 ``inspectSource()`` 统一处理。
"""
from __future__ import annotations

from pathlib import Path

from mcap.reader import make_reader


def inspect_mcap(path: str) -> dict:
    """扫描 MCAP 文件，汇总各 Topic 的 ROS2 消息类型与消息条数。

    Args:
        path: MCAP 文件路径。

    Returns:
        ``{"topics": [{"name": topic, "msg_type": str|None, "message_count": int}, ...]}``

    Raises:
        FileNotFoundError: 文件不存在。
    """
    mcap_path = Path(path)
    if not mcap_path.exists():
        raise FileNotFoundError(f"mcap file not found: {path}")

    topics: dict[str, dict] = {}
    with mcap_path.open("rb") as f:
        reader = make_reader(f)
        summary = reader.get_summary()
        # 优先读 summary：有 channel/schema/statistics 时无需全文件扫描
        if summary and summary.channels and summary.schemas:
            schema_map = {s.id: s for s in summary.schemas.values()}
            for channel in summary.channels.values():
                schema = schema_map.get(channel.schema_id)
                msg_type = _schema_name(schema.name if schema else None)
                entry = topics.setdefault(
                    channel.topic,
                    {"name": channel.topic, "msg_type": msg_type, "message_count": 0},
                )
                if entry["msg_type"] is None and msg_type:
                    entry["msg_type"] = msg_type
            if summary.statistics:
                for (_channel_id, topic), count in _channel_counts(reader).items():
                    if topic in topics:
                        topics[topic]["message_count"] = count
        else:
            # 无 summary 时回退：逐条 iter_messages 计数（较慢但兼容旧/不完整 MCAP）
            for _schema, channel, _message in reader.iter_messages():
                entry = topics.setdefault(
                    channel.topic,
                    {"name": channel.topic, "msg_type": None, "message_count": 0},
                )
                entry["message_count"] += 1

    return {"topics": list(topics.values())}


def _schema_name(name: str | None) -> str | None:
    """从 MCAP schema 名提取 ROS2 msg 全名（如 ``ros2msg ... pkg/msg/Type`` → ``pkg/msg/Type``）。"""
    if not name:
        return None
    if name.startswith("ros2msg"):
        return name.split(" ", 1)[-1]
    return name


def _channel_counts(reader) -> dict[tuple[int, str], int]:
    """逐条遍历消息，统计每个 (channel_id, topic) 的消息数（summary 无 statistics 时的回退）。"""
    counts: dict[tuple[int, str], int] = {}
    for _schema, channel, _message in reader.iter_messages():
        key = (channel.id, channel.topic)
        counts[key] = counts.get(key, 0) + 1
    return counts
