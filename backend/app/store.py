"""数据源元信息存储与插件注册表。

架构位置：位于 API 层（``main.py``）下方的「元信息层」。负责两件事：
  1. ``PLUGIN_META`` —— 静态声明各数据源插件类型及其能力（capabilities），
     供 API 层按能力分派，而非硬编码 plugin_type 字符串比较。
  2. ``DatasourceStore`` —— 以 JSON 文件（``data/datasources.json``）持久化数据源配置
     与导入状态，并管理每个数据源对应的 DuckDB 文件路径。

不含查询 / 导入逻辑：SQL 查询在 ``duckdb_engine``，文件导入在 ``ingest``。
全局单例 ``store`` 供整个后端共享。
"""
from __future__ import annotations

import json
import os
from pathlib import Path
from typing import Any

# 数据根目录：默认 backend/data，可用 PULSEVIEW_DATA_DIR 覆盖（测试用隔离目录）
_DEFAULT_DATA_DIR = Path(__file__).resolve().parent.parent / "data"
DATA_DIR = Path(os.environ.get("PULSEVIEW_DATA_DIR", str(_DEFAULT_DATA_DIR)))
META_FILE = DATA_DIR / "datasources.json"  # 数据源配置 + 导入状态
DUCKDB_DIR = DATA_DIR / "duckdb"  # 每个数据源一个 {id}.duckdb

# capabilities 声明插件支持的能力，API 层据此分派而非比较 plugin_type 字符串：
#   ingest  - 支持把原始文件导入 DuckDB
#   schema  - 支持返回 DuckDB 表结构
#   sql     - 支持 SQL 查询
#   promql  - 支持 PromQL 查询
PLUGIN_META = {
    "sqlite": {
        "plugin_type_name": "SQLite",
        "category": "timeseries",
        "capabilities": ["promql"],
    },
    "ros2_mcap": {
        "plugin_type_name": "ROS2 MCAP",
        "category": "ros2",
        "capabilities": ["ingest", "schema", "sql"],
    },
    "protobuf": {
        "plugin_type_name": "Protobuf",
        "category": "protobuf",
        "capabilities": ["ingest", "schema", "sql"],
    },
    "ctf": {
        "plugin_type_name": "CTF Trace",
        "category": "tracing",
        "capabilities": ["ingest", "schema", "sql"],
    },
    "perfetto": {
        "plugin_type_name": "Perfetto Trace",
        "category": "tracing",
        "capabilities": ["ingest", "schema", "sql"],
    },
}


def plugin_capabilities(plugin_type: str) -> set[str]:
    """返回插件声明的能力集合；未知插件类型返回空集合。"""
    return set(PLUGIN_META.get(plugin_type, {}).get("capabilities", []))


class DatasourceStore:
    """基于 JSON 文件的数据源 CRUD 存储（无数据库依赖，进程间通过文件共享）。

    每条数据源记录含：id、name、description、plugin_type、settings、is_default、
    ingest_status（pending/running/ready/error）、ingest_info（导入摘要或错误）。
    """

    def __init__(self) -> None:
        """确保数据目录与 DuckDB 目录存在，并初始化空的元数据文件。"""
        DATA_DIR.mkdir(parents=True, exist_ok=True)
        DUCKDB_DIR.mkdir(parents=True, exist_ok=True)
        if not META_FILE.exists():
            META_FILE.write_text("[]", encoding="utf-8")

    def _load(self) -> list[dict[str, Any]]:
        """从 JSON 文件读取全部数据源记录。"""
        return json.loads(META_FILE.read_text(encoding="utf-8"))

    def _save(self, items: list[dict[str, Any]]) -> None:
        """将全部数据源记录写回 JSON 文件。"""
        META_FILE.write_text(json.dumps(items, ensure_ascii=False, indent=2), encoding="utf-8")

    def list(self) -> list[dict[str, Any]]:
        """返回全部数据源记录。"""
        return self._load()

    def get(self, ds_id: int) -> dict[str, Any] | None:
        """按 id 查找单个数据源；不存在返回 None。"""
        return next((d for d in self._load() if d["id"] == ds_id), None)

    def create(self, payload: dict[str, Any]) -> dict[str, Any]:
        """新建数据源记录。

        自增 id；从 ``PLUGIN_META`` 补全 plugin_type_name；首个数据源默认设为
        is_default；初始 ingest_status 为 pending（实际导入由 API 层触发）。
        """
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
        """更新数据源；忽略 id 与 plugin_type（类型不可变）。不存在则抛 KeyError。"""
        items = self._load()
        for idx, item in enumerate(items):
            if item["id"] == ds_id:
                item.update({k: v for k, v in payload.items() if k not in ("id", "plugin_type")})
                items[idx] = item
                self._save(items)
                return item
        raise KeyError(f"datasource {ds_id} not found")

    def delete(self, ds_id: int) -> None:
        """删除数据源记录，并清理其对应的 DuckDB 文件。"""
        items = [d for d in self._load() if d["id"] != ds_id]
        self._save(items)
        db_path = self.duckdb_path(ds_id)
        if db_path.exists():
            db_path.unlink()

    def duckdb_path(self, ds_id: int) -> Path:
        """返回该数据源的 DuckDB 文件路径（``duckdb/{id}.duckdb``）。"""
        return DUCKDB_DIR / f"{ds_id}.duckdb"

    def set_ingest_status(self, ds_id: int, status: str, info: dict[str, Any] | None = None) -> None:
        """更新数据源的导入状态与摘要（由 API 层在 ingest 前后调用）。"""
        items = self._load()
        for item in items:
            if item["id"] == ds_id:
                item["ingest_status"] = status
                if info is not None:
                    item["ingest_info"] = info
        self._save(items)


store = DatasourceStore()
