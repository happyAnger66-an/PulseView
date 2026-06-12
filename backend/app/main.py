from __future__ import annotations

import random
import time
from typing import Any

from fastapi import FastAPI, HTTPException, Query
from fastapi.middleware.cors import CORSMiddleware

import app.ingest.adapters  # noqa: F401
from app.duckdb_engine import get_schema, run_query
from app.ingest.pipeline import ingest_mcap
from app.mcap_inspect import inspect_mcap
from app.models import ApiResponse, DatasourceCreate, DatasourceUpdate, IngestRequest, SqlQueryRequest
from app.store import PLUGIN_META, store

app = FastAPI(title="PulseView Backend")

app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)


def ok(data: Any = None) -> ApiResponse:
    """包装 API 成功响应为统一格式 ``{dat, err}``。

    Args:
        data: 业务数据，可为任意 JSON 可序列化对象。

    Returns:
        ``ApiResponse``，``err`` 为空字符串。
    """
    return ApiResponse(dat=data)


@app.get("/api/datasource/plugins")
def list_plugins():
    """列出支持的数据源插件类型及元信息。

    Returns:
        ``ApiResponse``，``dat`` 为插件列表（含 ``plugin_type``、``plugin_type_name``、``category``）。
    """
    return ok(
        [
            {"plugin_type": k, **v}
            for k, v in PLUGIN_META.items()
        ]
    )


@app.get("/api/datasources")
def list_datasources():
    """列出全部已配置数据源。

    Returns:
        ``ApiResponse``，``dat`` 为数据源对象数组。
    """
    return ok(store.list())


@app.post("/api/datasources")
def create_datasource(body: DatasourceCreate):
    """创建数据源；ros2_mcap 类型会自动触发首次 ingest。

    Args:
        body: 数据源名称、插件类型、settings 等创建参数。

    Returns:
        ``ApiResponse``，``dat`` 为创建后的数据源对象（含 ingest 状态）。
    """
    item = store.create(body.model_dump())
    if body.plugin_type == "ros2_mcap":
        _try_ingest(item["id"])
        item = store.get(item["id"])
    return ok(item)


@app.get("/api/datasources/{ds_id}")
def get_datasource(ds_id: int):
    """按 ID 获取单个数据源。

    Args:
        ds_id: 数据源 ID。

    Returns:
        ``ApiResponse``，``dat`` 为数据源对象。

    Raises:
        HTTPException: 404，数据源不存在。
    """
    item = store.get(ds_id)
    if not item:
        raise HTTPException(status_code=404, detail="not found")
    return ok(item)


@app.put("/api/datasources/{ds_id}")
def update_datasource(ds_id: int, body: DatasourceUpdate):
    """更新数据源配置；ros2_mcap 类型会重新尝试 ingest。

    Args:
        ds_id: 数据源 ID。
        body: 待更新的字段（name、description、settings 等，仅传非空字段）。

    Returns:
        ``ApiResponse``，``dat`` 为更新后的数据源对象。

    Raises:
        HTTPException: 404，数据源不存在。
    """
    try:
        item = store.update(ds_id, body.model_dump(exclude_none=True))
    except KeyError:
        raise HTTPException(status_code=404, detail="not found") from None
    if item["plugin_type"] == "ros2_mcap":
        _try_ingest(ds_id)
        item = store.get(ds_id)
    return ok(item)


@app.delete("/api/datasources/{ds_id}")
def delete_datasource(ds_id: int):
    """删除数据源及其关联 DuckDB 文件。

    Args:
        ds_id: 数据源 ID。

    Returns:
        ``ApiResponse``，``dat`` 为 ``"ok"``。
    """
    store.delete(ds_id)
    return ok("ok")


@app.get("/api/mcap/inspect")
def mcap_inspect(path: str = Query(...)):
    """扫描 MCAP 文件，返回 topic 列表及消息类型、条数。

    Args:
        path: MCAP 文件绝对路径。

    Returns:
        ``ApiResponse``，``dat`` 含 ``topics`` 数组。

    Raises:
        HTTPException: 404，文件不存在。
    """
    try:
        return ok(inspect_mcap(path))
    except FileNotFoundError as e:
        raise HTTPException(status_code=404, detail=str(e)) from e


@app.post("/api/datasources/{ds_id}/ingest")
def ingest_datasource(ds_id: int, body: IngestRequest | None = None):
    """手动触发 ros2_mcap 数据源的 MCAP 导入。

    Args:
        ds_id: 数据源 ID。
        body: 可选；``force=True`` 时强制重新导入。

    Returns:
        ``ApiResponse``，``dat`` 为 ingest 结果（status、messages_decoded 等）。

    Raises:
        HTTPException: 404 数据源不存在；400 非 ros2_mcap 类型。
    """
    item = store.get(ds_id)
    if not item:
        raise HTTPException(status_code=404, detail="not found")
    if item["plugin_type"] != "ros2_mcap":
        raise HTTPException(status_code=400, detail="only ros2_mcap supports ingest")
    info = _try_ingest(ds_id, force=body.force if body else False)
    return ok(info)


@app.get("/api/datasources/{ds_id}/schema")
def datasource_schema(ds_id: int):
    """返回数据源 DuckDB 表结构及 Adapter 元数据（dimension_keys 等）。

    Args:
        ds_id: 数据源 ID。

    Returns:
        ``ApiResponse``，``dat`` 含 ``tables`` 数组；非 ros2_mcap 返回空表列表。

    Raises:
        HTTPException: 404，数据源不存在。
    """
    item = store.get(ds_id)
    if not item:
        raise HTTPException(status_code=404, detail="not found")
    if item["plugin_type"] != "ros2_mcap":
        return ok({"tables": []})
    msg_type = item.get("settings", {}).get("mcap.msg_type")
    return ok(get_schema(store.duckdb_path(ds_id), msg_type))


@app.post("/api/sql/query")
def sql_query(body: SqlQueryRequest):
    """对 ros2_mcap 数据源的 DuckDB 执行 SELECT 查询。

    Args:
        body: 含 ``datasource_id``、``sql``、``limit``（默认 1000）。

    Returns:
        ``ApiResponse``，``dat`` 含 ``columns``、``rows``、``meta``（时间/维度/数值列推断）。

    Raises:
        HTTPException: 404 数据源不存在；400 非 ros2_mcap、DuckDB 未就绪或 SQL 非法。
    """
    item = store.get(body.datasource_id)
    if not item:
        raise HTTPException(status_code=404, detail="datasource not found")
    if item["plugin_type"] != "ros2_mcap":
        raise HTTPException(status_code=400, detail="sql query only supported for ros2_mcap")
    try:
        return ok(run_query(store.duckdb_path(body.datasource_id), body.sql, body.limit))
    except FileNotFoundError as e:
        raise HTTPException(status_code=400, detail=str(e)) from e
    except ValueError as e:
        raise HTTPException(status_code=400, detail=str(e)) from e


@app.post("/api/query_range")
def promql_query_range(body: dict[str, Any]):
    """Prometheus 兼容的 query_range 占位接口，返回模拟时序数据。

    Args:
        body: 含 ``query``、``start``、``end``、``step``（均为可选，有默认值）。

    Returns:
        ``ApiResponse``，``dat`` 为 Prometheus matrix 格式（status、data.result）。
    """
    query = body.get("query", "up")
    start = int(body.get("start", time.time() - 3600))
    end = int(body.get("end", time.time()))
    step = int(body.get("step", 15))
    points = []
    t = start
    while t <= end:
        val = 50 + 50 * __import__("math").sin(t / 300) + random.random() * 5
        points.append([t, f"{val:.2f}"])
        t += step
    name = query.split()[0] if query else "metric"
    return ok(
        {
            "status": "success",
            "data": {
                "resultType": "matrix",
                "result": [{"metric": {"__name__": name}, "values": points}],
            },
        }
    )


def _try_ingest(ds_id: int, force: bool = False) -> dict[str, Any]:
    """对 ros2_mcap 数据源执行 MCAP → DuckDB 导入，并更新 ingest 状态。

    Args:
        ds_id: 数据源 ID。
        force: 为 True 时忽略已有 DuckDB 文件并重新导入；为 False 时若已 ready 则跳过。

    Returns:
        导入结果摘要（含 status、messages_decoded 等）；非 ros2_mcap 或数据源不存在时返回空 dict。
    """
    item = store.get(ds_id)
    if not item or item["plugin_type"] != "ros2_mcap":
        return {}
    settings = item.get("settings", {})
    mcap_path = settings.get("mcap.path")
    topic = settings.get("mcap.topic")
    msg_type = settings.get("mcap.msg_type")
    if not mcap_path or not topic or not msg_type:
        store.set_ingest_status(ds_id, "pending", {"message": "missing mcap.path/topic/msg_type"})
        return {"status": "pending"}

    db_path = store.duckdb_path(ds_id)
    if db_path.exists() and not force:
        current = store.get(ds_id)
        if current and current.get("ingest_status") == "ready":
            store.set_ingest_status(ds_id, "ready", {"message": "duckdb exists, skip ingest"})
            return {"status": "ready", "skipped": True}

    store.set_ingest_status(ds_id, "running")
    try:
        info = ingest_mcap(db_path, __import__("pathlib").Path(mcap_path), topic, msg_type)
        if info.get("messages_decoded", 0) == 0:
            raise ValueError(f"no messages decoded for topic {topic}")
        info["status"] = "ready"
        store.set_ingest_status(ds_id, "ready", info)
        return info
    except Exception as e:
        if db_path.exists():
            db_path.unlink(missing_ok=True)
        store.set_ingest_status(ds_id, "error", {"message": str(e)})
        return {"status": "error", "message": str(e)}
