from __future__ import annotations

from typing import Any

from pydantic import BaseModel, Field


class ApiResponse(BaseModel):
    dat: Any = None
    err: str = ""


class DatasourceCreate(BaseModel):
    # 插件类型不再用 Literal 枚举写死，合法性由 API 层对照 PLUGIN_META 校验
    plugin_type: str
    name: str
    description: str = ""
    settings: dict[str, Any] = Field(default_factory=dict)
    is_default: bool = False


class DatasourceUpdate(BaseModel):
    name: str | None = None
    description: str | None = None
    settings: dict[str, Any] | None = None


class SqlQueryRequest(BaseModel):
    datasource_id: int
    sql: str
    limit: int = 1000


class IngestRequest(BaseModel):
    force: bool = False
