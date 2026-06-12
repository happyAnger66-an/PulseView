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
    return ApiResponse(dat=data)


@app.get("/api/datasource/plugins")
def list_plugins():
    return ok(
        [
            {"plugin_type": k, **v}
            for k, v in PLUGIN_META.items()
        ]
    )


@app.get("/api/datasources")
def list_datasources():
    return ok(store.list())


@app.post("/api/datasources")
def create_datasource(body: DatasourceCreate):
    item = store.create(body.model_dump())
    if body.plugin_type == "ros2_mcap":
        _try_ingest(item["id"])
        item = store.get(item["id"])
    return ok(item)


@app.get("/api/datasources/{ds_id}")
def get_datasource(ds_id: int):
    item = store.get(ds_id)
    if not item:
        raise HTTPException(status_code=404, detail="not found")
    return ok(item)


@app.put("/api/datasources/{ds_id}")
def update_datasource(ds_id: int, body: DatasourceUpdate):
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
    store.delete(ds_id)
    return ok("ok")


@app.get("/api/mcap/inspect")
def mcap_inspect(path: str = Query(...)):
    try:
        return ok(inspect_mcap(path))
    except FileNotFoundError as e:
        raise HTTPException(status_code=404, detail=str(e)) from e


@app.post("/api/datasources/{ds_id}/ingest")
def ingest_datasource(ds_id: int, body: IngestRequest | None = None):
    item = store.get(ds_id)
    if not item:
        raise HTTPException(status_code=404, detail="not found")
    if item["plugin_type"] != "ros2_mcap":
        raise HTTPException(status_code=400, detail="only ros2_mcap supports ingest")
    info = _try_ingest(ds_id, force=body.force if body else False)
    return ok(info)


@app.get("/api/datasources/{ds_id}/schema")
def datasource_schema(ds_id: int):
    item = store.get(ds_id)
    if not item:
        raise HTTPException(status_code=404, detail="not found")
    if item["plugin_type"] != "ros2_mcap":
        return ok({"tables": []})
    return ok(get_schema(store.duckdb_path(ds_id)))


@app.post("/api/sql/query")
def sql_query(body: SqlQueryRequest):
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
