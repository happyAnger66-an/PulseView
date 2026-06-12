from __future__ import annotations

from typing import Any, Literal

from pydantic import BaseModel, Field


class ApiResponse(BaseModel):
    dat: Any = None
    err: str = ""


class DatasourceCreate(BaseModel):
    name: str
    description: str = ""
    plugin_type: Literal["sqlite", "ros2_mcap"]
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
