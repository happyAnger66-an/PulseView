from __future__ import annotations

from typing import Any


def ros_msg_to_dict(obj: Any) -> Any:
    """Recursively convert mcap_ros2 dynamic message objects to plain dicts."""
    if obj is None:
        return None
    if isinstance(obj, (str, int, float, bool)):
        return obj
    if isinstance(obj, bytes):
        return obj.decode("utf-8", errors="replace")
    if isinstance(obj, (list, tuple)):
        return [ros_msg_to_dict(item) for item in obj]
    if isinstance(obj, dict):
        return {key: ros_msg_to_dict(value) for key, value in obj.items()}

    slots = getattr(obj, "__slots__", None)
    if slots:
        return {name: ros_msg_to_dict(getattr(obj, name)) for name in slots}

    if hasattr(obj, "__dict__"):
        return {key: ros_msg_to_dict(value) for key, value in obj.__dict__.items()}

    return obj
