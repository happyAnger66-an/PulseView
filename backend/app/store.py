from __future__ import annotations

import json
import os
from pathlib import Path
from typing import Any

_DEFAULT_DATA_DIR = Path(__file__).resolve().parent.parent / "data"
DATA_DIR = Path(os.environ.get("PULSEVIEW_DATA_DIR", str(_DEFAULT_DATA_DIR)))
META_FILE = DATA_DIR / "datasources.json"
DUCKDB_DIR = DATA_DIR / "duckdb"

PLUGIN_META = {
    "sqlite": {"plugin_type_name": "SQLite", "category": "timeseries"},
    "ros2_mcap": {"plugin_type_name": "ROS2 MCAP", "category": "ros2"},
}


class DatasourceStore:
    def __init__(self) -> None:
        DATA_DIR.mkdir(parents=True, exist_ok=True)
        DUCKDB_DIR.mkdir(parents=True, exist_ok=True)
        if not META_FILE.exists():
            META_FILE.write_text("[]", encoding="utf-8")

    def _load(self) -> list[dict[str, Any]]:
        return json.loads(META_FILE.read_text(encoding="utf-8"))

    def _save(self, items: list[dict[str, Any]]) -> None:
        META_FILE.write_text(json.dumps(items, ensure_ascii=False, indent=2), encoding="utf-8")

    def list(self) -> list[dict[str, Any]]:
        return self._load()

    def get(self, ds_id: int) -> dict[str, Any] | None:
        return next((d for d in self._load() if d["id"] == ds_id), None)

    def create(self, payload: dict[str, Any]) -> dict[str, Any]:
        items = self._load()
        ds_id = max([d["id"] for d in items], default=0) + 1
        plugin_type = payload["plugin_type"]
        meta = PLUGIN_META.get(plugin_type, {"plugin_type_name": plugin_type, "category": "unknown"})
        item = {
            "id": ds_id,
            "name": payload["name"],
            "description": payload.get("description", ""),
            "plugin_type": plugin_type,
            "plugin_type_name": meta["plugin_type_name"],
            "settings": payload.get("settings", {}),
            "is_default": payload.get("is_default", len(items) == 0),
            "ingest_status": "pending",
            "ingest_info": {},
        }
        items.append(item)
        self._save(items)
        return item

    def update(self, ds_id: int, payload: dict[str, Any]) -> dict[str, Any]:
        items = self._load()
        for idx, item in enumerate(items):
            if item["id"] == ds_id:
                item.update({k: v for k, v in payload.items() if k not in ("id", "plugin_type")})
                items[idx] = item
                self._save(items)
                return item
        raise KeyError(f"datasource {ds_id} not found")

    def delete(self, ds_id: int) -> None:
        items = [d for d in self._load() if d["id"] != ds_id]
        self._save(items)
        db_path = self.duckdb_path(ds_id)
        if db_path.exists():
            db_path.unlink()

    def duckdb_path(self, ds_id: int) -> Path:
        return DUCKDB_DIR / f"{ds_id}.duckdb"

    def set_ingest_status(self, ds_id: int, status: str, info: dict[str, Any] | None = None) -> None:
        items = self._load()
        for item in items:
            if item["id"] == ds_id:
                item["ingest_status"] = status
                if info is not None:
                    item["ingest_info"] = info
        self._save(items)


store = DatasourceStore()
