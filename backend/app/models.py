"""API 请求 / 响应数据模型（Pydantic Schema）。

架构位置：位于 API 层（``main.py``）与外部前端之间，是 FastAPI 的「数据契约」边界。
所有 HTTP 入参经此校验/反序列化，出参经 ``ApiResponse`` 统一包装；不含任何业务逻辑，
业务处理由 ``main.py`` 调用 ``store`` / ``ingest`` / ``duckdb_engine`` 完成。
"""
from __future__ import annotations

from typing import Any

from pydantic import BaseModel, Field


class ApiResponse(BaseModel):
    """所有接口的统一响应包装。

    ``dat`` 承载业务数据（任意 JSON 可序列化对象），``err`` 为错误信息（成功时为空串），
    使前端可用固定结构 ``{dat, err}`` 解析。
    """

    dat: Any = None
    err: str = ""


class DatasourceCreate(BaseModel):
    """创建数据源的请求体。

    ``plugin_type`` 标识数据源类型（如 ros2_mcap / protobuf / ctf / sqlite）；不再用
    Literal 枚举写死，合法性由 API 层对照 ``PLUGIN_META`` 校验，便于扩展新格式。
    ``settings`` 为各格式专属配置（如 mcap.path、proto.msg_type、ctf.path）。
    """

    plugin_type: str
    name: str
    description: str = ""
    settings: dict[str, Any] = Field(default_factory=dict)
    is_default: bool = False


class DatasourceUpdate(BaseModel):
    """更新数据源的请求体；字段均可选，仅更新提供的部分（部分更新语义）。"""

    name: str | None = None
    description: str | None = None
    settings: dict[str, Any] | None = None


class SqlQueryRequest(BaseModel):
    """SQL 查询请求体，作用于已导入 DuckDB 的数据源。

    ``datasource_id`` 指定目标数据源，``sql`` 仅允许 SELECT（由查询层强制），
    ``limit`` 限制返回行数以保护后端。
    """

    datasource_id: int
    sql: str
    limit: int = 1000


class IngestRequest(BaseModel):
    """触发导入的请求体；``force=True`` 时即使已导入也强制重新导入。"""

    force: bool = False
