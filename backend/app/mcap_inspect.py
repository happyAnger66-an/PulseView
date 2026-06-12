from __future__ import annotations

from pathlib import Path

from mcap.reader import make_reader


def inspect_mcap(path: str) -> dict:
    mcap_path = Path(path)
    if not mcap_path.exists():
        raise FileNotFoundError(f"mcap file not found: {path}")

    topics: dict[str, dict] = {}
    with mcap_path.open("rb") as f:
        reader = make_reader(f)
        summary = reader.get_summary()
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
            for _schema, channel, _message in reader.iter_messages():
                entry = topics.setdefault(
                    channel.topic,
                    {"name": channel.topic, "msg_type": None, "message_count": 0},
                )
                entry["message_count"] += 1

    return {"topics": list(topics.values())}


def _schema_name(name: str | None) -> str | None:
    if not name:
        return None
    if name.startswith("ros2msg"):
        return name.split(" ", 1)[-1]
    return name


def _channel_counts(reader) -> dict[tuple[int, str], int]:
    counts: dict[tuple[int, str], int] = {}
    for _schema, channel, _message in reader.iter_messages():
        key = (channel.id, channel.topic)
        counts[key] = counts.get(key, 0) + 1
    return counts
