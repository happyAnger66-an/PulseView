from __future__ import annotations

from typing import Any

from app.ingest.registry import ColumnDef, IngestContext, RosMsgAdapter, TableDef, registry

MSG_TYPE = "system_stats_interfaces/msg/SystemStats"


def _stamp_to_us(header: dict[str, Any] | None) -> int:
    if not header:
        return 0
    stamp = header.get("stamp") or {}
    sec = int(stamp.get("sec", 0))
    nanosec = int(stamp.get("nanosec", 0))
    return sec * 1_000_000 + nanosec // 1000


def _scalar_row(msg: dict[str, Any], ctx: IngestContext) -> dict[str, Any]:
    header = msg.get("header") or {}
    return {
        "msg_id": ctx.msg_id,
        "_time": ctx.time_us,
        "frame_id": header.get("frame_id"),
        "seq": header.get("seq"),
        "hostname": msg.get("hostname"),
        "vin": msg.get("vin"),
        "cpu_used_percent": msg.get("cpu_used_percent"),
        "mem_free_size": msg.get("mem_free_size"),
        "mem_total_size": msg.get("mem_total_size"),
        "mem_used_percent": msg.get("mem_used_percent"),
    }


def _expand_array(msg_id: int, items: list[dict[str, Any]] | None, mapper) -> list[dict[str, Any]]:
    rows: list[dict[str, Any]] = []
    for idx, item in enumerate(items or []):
        row = mapper(item)
        row["msg_id"] = msg_id
        row["idx"] = idx
        rows.append(row)
    return rows


class SystemStatsAdapter:
    msg_type = MSG_TYPE

    def tables(self) -> list[TableDef]:
        return [
            TableDef(
                "system_stats",
                [
                    ColumnDef("msg_id", "BIGINT"),
                    ColumnDef("_time", "BIGINT"),
                    ColumnDef("frame_id", "VARCHAR"),
                    ColumnDef("seq", "UINTEGER"),
                    ColumnDef("hostname", "VARCHAR"),
                    ColumnDef("vin", "VARCHAR"),
                    ColumnDef("cpu_used_percent", "FLOAT"),
                    ColumnDef("mem_free_size", "FLOAT"),
                    ColumnDef("mem_total_size", "FLOAT"),
                    ColumnDef("mem_used_percent", "FLOAT"),
                ],
            ),
            TableDef(
                "system_stats_cpu_stats",
                [
                    ColumnDef("msg_id", "BIGINT"),
                    ColumnDef("idx", "UINTEGER"),
                    ColumnDef("cpu_name", "VARCHAR"),
                    ColumnDef("cpu_percent", "FLOAT"),
                    ColumnDef("cpu_user_percent", "FLOAT"),
                    ColumnDef("cpu_sys_percent", "FLOAT"),
                    ColumnDef("cpu_idle_percent", "FLOAT"),
                ],
            ),
            TableDef(
                "system_stats_gpu_stats",
                [
                    ColumnDef("msg_id", "BIGINT"),
                    ColumnDef("idx", "UINTEGER"),
                    ColumnDef("name", "VARCHAR"),
                    ColumnDef("gpu_usage", "FLOAT"),
                ],
            ),
            TableDef(
                "system_stats_proc_stats",
                [
                    ColumnDef("msg_id", "BIGINT"),
                    ColumnDef("idx", "UINTEGER"),
                    ColumnDef("name", "VARCHAR"),
                    ColumnDef("pid", "INTEGER"),
                    ColumnDef("cpu_used_percent", "FLOAT"),
                    ColumnDef("mem_used_percent", "FLOAT"),
                    ColumnDef("status", "VARCHAR"),
                ],
            ),
            TableDef(
                "system_stats_net_stats",
                [
                    ColumnDef("msg_id", "BIGINT"),
                    ColumnDef("idx", "UINTEGER"),
                    ColumnDef("name", "VARCHAR"),
                    ColumnDef("send_rate", "FLOAT"),
                    ColumnDef("rcv_rate", "FLOAT"),
                    ColumnDef("status", "VARCHAR"),
                ],
            ),
            TableDef(
                "system_stats_filesystem_stats",
                [
                    ColumnDef("msg_id", "BIGINT"),
                    ColumnDef("idx", "UINTEGER"),
                    ColumnDef("mount_point", "VARCHAR"),
                    ColumnDef("used_percent", "FLOAT"),
                    ColumnDef("total", "FLOAT"),
                    ColumnDef("used", "FLOAT"),
                    ColumnDef("free", "FLOAT"),
                ],
            ),
            TableDef(
                "system_stats_mem_detail_stat",
                [
                    ColumnDef("msg_id", "BIGINT"),
                    ColumnDef("active", "FLOAT"),
                    ColumnDef("inactive", "FLOAT"),
                    ColumnDef("dirty", "FLOAT"),
                    ColumnDef("mapped", "FLOAT"),
                    ColumnDef("anon_pages", "FLOAT"),
                ],
            ),
            TableDef(
                "system_stats_node_pub_stats",
                [
                    ColumnDef("msg_id", "BIGINT"),
                    ColumnDef("idx", "UINTEGER"),
                    ColumnDef("node", "VARCHAR"),
                    ColumnDef("topic", "VARCHAR"),
                    ColumnDef("hz", "FLOAT"),
                    ColumnDef("min_delta", "FLOAT"),
                    ColumnDef("max_delta", "FLOAT"),
                    ColumnDef("avg_delta", "FLOAT"),
                    ColumnDef("min_delta_ts", "BIGINT"),
                    ColumnDef("max_delta_ts", "BIGINT"),
                    ColumnDef("min_proc_delta", "FLOAT"),
                    ColumnDef("max_proc_delta", "FLOAT"),
                    ColumnDef("avg_proc_delta", "FLOAT"),
                    ColumnDef("min_proc_delta_ts", "BIGINT"),
                    ColumnDef("max_proc_delta_ts", "BIGINT"),
                    ColumnDef("data_ts", "BIGINT"),
                ],
            ),
            TableDef(
                "system_stats_node_sub_stats",
                [
                    ColumnDef("msg_id", "BIGINT"),
                    ColumnDef("idx", "UINTEGER"),
                    ColumnDef("node", "VARCHAR"),
                    ColumnDef("topic", "VARCHAR"),
                    ColumnDef("hz", "FLOAT"),
                    ColumnDef("min_delta", "FLOAT"),
                    ColumnDef("max_delta", "FLOAT"),
                    ColumnDef("avg_delta", "FLOAT"),
                    ColumnDef("min_delta_ts", "BIGINT"),
                    ColumnDef("max_delta_ts", "BIGINT"),
                    ColumnDef("min_proc_delta", "FLOAT"),
                    ColumnDef("max_proc_delta", "FLOAT"),
                    ColumnDef("avg_proc_delta", "FLOAT"),
                    ColumnDef("min_proc_delta_ts", "BIGINT"),
                    ColumnDef("max_proc_delta_ts", "BIGINT"),
                    ColumnDef("min_sched_delta", "FLOAT"),
                    ColumnDef("max_sched_delta", "FLOAT"),
                    ColumnDef("avg_sched_delta", "FLOAT"),
                    ColumnDef("min_sched_delta_ts", "BIGINT"),
                    ColumnDef("max_sched_delta_ts", "BIGINT"),
                    ColumnDef("data_ts", "BIGINT"),
                    ColumnDef("min_ipc", "FLOAT"),
                    ColumnDef("max_ipc", "FLOAT"),
                    ColumnDef("avg_ipc", "FLOAT"),
                    ColumnDef("min_ipc_ts", "BIGINT"),
                    ColumnDef("max_ipc_ts", "BIGINT"),
                ],
            ),
        ]

    def flatten(self, msg: dict[str, Any], ctx: IngestContext) -> dict[str, list[dict[str, Any]]]:
        msg_id = ctx.msg_id
        rows: dict[str, list[dict[str, Any]]] = {
            "system_stats": [_scalar_row(msg, ctx)],
            "system_stats_cpu_stats": _expand_array(
                msg_id,
                msg.get("cpu_stats"),
                lambda x: {
                    "cpu_name": x.get("cpu_name"),
                    "cpu_percent": x.get("cpu_percent"),
                    "cpu_user_percent": x.get("cpu_user_percent"),
                    "cpu_sys_percent": x.get("cpu_sys_percent"),
                    "cpu_idle_percent": x.get("cpu_idle_percent"),
                },
            ),
            "system_stats_gpu_stats": _expand_array(
                msg_id,
                msg.get("gpu_stats"),
                lambda x: {"name": x.get("name"), "gpu_usage": x.get("gpu_usage")},
            ),
            "system_stats_proc_stats": _expand_array(
                msg_id,
                msg.get("proc_stats"),
                lambda x: {
                    "name": x.get("name"),
                    "pid": x.get("pid"),
                    "cpu_used_percent": x.get("cpu_used_percent"),
                    "mem_used_percent": x.get("mem_used_percent"),
                    "status": x.get("status"),
                },
            ),
            "system_stats_net_stats": _expand_array(
                msg_id,
                msg.get("net_stats"),
                lambda x: {
                    "name": x.get("name"),
                    "send_rate": x.get("send_rate"),
                    "rcv_rate": x.get("rcv_rate"),
                    "status": x.get("status"),
                },
            ),
            "system_stats_filesystem_stats": _expand_array(
                msg_id,
                msg.get("filesystem_stats"),
                lambda x: {
                    "mount_point": x.get("mount_point"),
                    "used_percent": x.get("used_percent"),
                    "total": x.get("total"),
                    "used": x.get("used"),
                    "free": x.get("free"),
                },
            ),
            "system_stats_node_pub_stats": _expand_array(
                msg_id,
                msg.get("node_pub_stats"),
                lambda x: {
                    "node": x.get("node"),
                    "topic": x.get("topic"),
                    "hz": x.get("hz"),
                    "min_delta": x.get("min_delta"),
                    "max_delta": x.get("max_delta"),
                    "avg_delta": x.get("avg_delta"),
                    "min_delta_ts": x.get("min_delta_ts"),
                    "max_delta_ts": x.get("max_delta_ts"),
                    "min_proc_delta": x.get("min_proc_delta"),
                    "max_proc_delta": x.get("max_proc_delta"),
                    "avg_proc_delta": x.get("avg_proc_delta"),
                    "min_proc_delta_ts": x.get("min_proc_delta_ts"),
                    "max_proc_delta_ts": x.get("max_proc_delta_ts"),
                    "data_ts": x.get("data_ts"),
                },
            ),
            "system_stats_node_sub_stats": _expand_array(
                msg_id,
                msg.get("node_sub_stats"),
                lambda x: {
                    "node": x.get("node"),
                    "topic": x.get("topic"),
                    "hz": x.get("hz"),
                    "min_delta": x.get("min_delta"),
                    "max_delta": x.get("max_delta"),
                    "avg_delta": x.get("avg_delta"),
                    "min_delta_ts": x.get("min_delta_ts"),
                    "max_delta_ts": x.get("max_delta_ts"),
                    "min_proc_delta": x.get("min_proc_delta"),
                    "max_proc_delta": x.get("max_proc_delta"),
                    "avg_proc_delta": x.get("avg_proc_delta"),
                    "min_proc_delta_ts": x.get("min_proc_delta_ts"),
                    "max_proc_delta_ts": x.get("max_proc_delta_ts"),
                    "min_sched_delta": x.get("min_sched_delta"),
                    "max_sched_delta": x.get("max_sched_delta"),
                    "avg_sched_delta": x.get("avg_sched_delta"),
                    "min_sched_delta_ts": x.get("min_sched_delta_ts"),
                    "max_sched_delta_ts": x.get("max_sched_delta_ts"),
                    "data_ts": x.get("data_ts"),
                    "min_ipc": x.get("min_ipc"),
                    "max_ipc": x.get("max_ipc"),
                    "avg_ipc": x.get("avg_ipc"),
                    "min_ipc_ts": x.get("min_ipc_ts"),
                    "max_ipc_ts": x.get("max_ipc_ts"),
                },
            ),
        }

        mem_detail = msg.get("mem_detail_stat")
        if isinstance(mem_detail, dict):
            rows["system_stats_mem_detail_stat"] = [
                {
                    "msg_id": msg_id,
                    "active": mem_detail.get("active"),
                    "inactive": mem_detail.get("inactive"),
                    "dirty": mem_detail.get("dirty"),
                    "mapped": mem_detail.get("mapped"),
                    "anon_pages": mem_detail.get("anon_pages"),
                }
            ]
        else:
            rows["system_stats_mem_detail_stat"] = []

        return rows


registry.register(SystemStatsAdapter())
