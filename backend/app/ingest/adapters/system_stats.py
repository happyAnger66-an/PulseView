"""``system_stats_interfaces/msg/SystemStats`` 的 RosMsgAdapter 实现（参考模板）。

架构位置
--------
MCAP 导入管线（``ingest/pipeline.py``）在解码每条 ROS 消息后，按数据源配置的
``mcap.msg_type`` 从 ``registry`` 查找 Adapter，调用 ``flatten()`` 展平为多表行写入 DuckDB。
本文件是 **消息级** 扩展点：新增 ROS2 msg 类型时，复制本文件模式即可，无需改 MCAP Importer 或前端。

新增 msg 类型步骤（最小）
------------------------
1. 在本目录新建 ``your_msg.py``，定义 ``MSG_TYPE``（须与 MCAP schema / 建源表单一致）
2. 实现 ``YourMsgAdapter``：``tables()`` 声明表结构，``flatten()`` 映射字段
3. 文件末尾 ``registry.register(YourMsgAdapter())``
4. 在 ``adapters/__init__.py`` 增加 ``import app.ingest.adapters.your_msg``

表设计约定
----------
- **主表**：含 ``msg_id``、``_time``（微秒）；``parent_table`` 为空；``default_metrics`` 列出默认可画折线的标量列
- **子表**（数组字段）：含 ``msg_id``、``idx``；设 ``parent_table``、``dimension_keys``（分线维度）、``default_metrics``
- **单条 struct**（非数组）：可并入主表或单独子表（见 ``mem_detail_stat``），``flatten`` 里按 dict 处理

详见 ``docs/add_new_ros2_msg.md``。
"""
from __future__ import annotations

from typing import Any

from app.ingest.registry import ColumnDef, IngestContext, RosMsgAdapter, TableDef, registry

# 与 MCAP schema、数据源 settings['mcap.msg_type'] 必须完全一致
MSG_TYPE = "system_stats_interfaces/msg/SystemStats"


def _stamp_to_us(header: dict[str, Any] | None) -> int:
    """将 ROS ``header.stamp`` 转为微秒时间戳（备用；本 Adapter 主路径使用 ``ctx.time_us``）。"""
    if not header:
        return 0
    stamp = header.get("stamp") or {}
    sec = int(stamp.get("sec", 0))
    nanosec = int(stamp.get("nanosec", 0))
    return sec * 1_000_000 + nanosec // 1000


def _scalar_row(msg: dict[str, Any], ctx: IngestContext) -> dict[str, Any]:
    """提取主表一行：标量字段 + ``msg_id`` / ``_time``（来自 ``IngestContext``）。

    新 msg 类型：把消息顶层标量字段映射到与 ``tables()`` 主表列名一致的 dict。
    """
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
    """将消息中的**数组字段**展开为子表多行（long format）。

    每条元素经 ``mapper(item)`` 转为列 dict，并自动附加 ``msg_id``、``idx``。
    新 msg 类型：对 each ``msg.get("your_array")`` 调用本函数，``mapper`` 内字段名须与子表 ``ColumnDef`` 一致。
    """
    rows: list[dict[str, Any]] = []
    for idx, item in enumerate(items or []):
        row = mapper(item)
        row["msg_id"] = msg_id
        row["idx"] = idx
        rows.append(row)
    return rows


class SystemStatsAdapter:
    """``RosMsgAdapter`` 协议实现：SystemStats 消息 → DuckDB 主表 + 7 张子表。"""

    # registry 按此字符串匹配；与 MSG_TYPE、数据源配置三者须一致
    msg_type = MSG_TYPE

    def tables(self) -> list[TableDef]:
        """声明本 msg 对应的 DuckDB 表结构与可视化元数据。

        ingest 时用于 ``CREATE TABLE`` 并写入 ``_pv_table_meta``；Schema API / 前端
        ``sqlBuilder`` 据此生成 JOIN SQL 与预设查询。

        返回 ``list[TableDef]``，每张表说明：
        - 主表 ``system_stats``：``default_metrics`` → Explorer 预设按钮、默认折线指标
        - 子表 ``*_stats``：``parent_table`` + ``dimension_keys`` → 按 cpu_name/node+topic 等分线
        - 列类型用 DuckDB 类型名（``BIGINT``/``FLOAT``/``VARCHAR`` 等）

        新 msg：先规划主表/子表，再在此列出全部 ``TableDef``；列名与 ``flatten()`` 输出键一致。
        """
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
                default_metrics=["cpu_used_percent", "mem_used_percent"],
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
                parent_table="system_stats",
                dimension_keys=["cpu_name"],
                default_metrics=["cpu_percent", "cpu_user_percent", "cpu_sys_percent", "cpu_idle_percent"],
            ),
            TableDef(
                "system_stats_gpu_stats",
                [
                    ColumnDef("msg_id", "BIGINT"),
                    ColumnDef("idx", "UINTEGER"),
                    ColumnDef("name", "VARCHAR"),
                    ColumnDef("gpu_usage", "FLOAT"),
                ],
                parent_table="system_stats",
                dimension_keys=["name"],
                default_metrics=["gpu_usage"],
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
                parent_table="system_stats",
                dimension_keys=["name", "pid"],
                default_metrics=["cpu_used_percent", "mem_used_percent"],
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
                parent_table="system_stats",
                dimension_keys=["name"],
                default_metrics=["send_rate", "rcv_rate"],
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
                parent_table="system_stats",
                dimension_keys=["mount_point"],
                default_metrics=["used_percent", "total", "used", "free"],
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
                parent_table="system_stats",
                default_metrics=["active", "inactive", "dirty", "mapped", "anon_pages"],
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
                parent_table="system_stats",
                dimension_keys=["node", "topic"],
                default_metrics=["hz", "avg_delta", "avg_proc_delta"],
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
                parent_table="system_stats",
                dimension_keys=["node", "topic"],
                default_metrics=["hz", "avg_delta", "avg_sched_delta", "avg_ipc"],
            ),
        ]

    def flatten(self, msg: dict[str, Any], ctx: IngestContext) -> dict[str, list[dict[str, Any]]]:
        """将单条解码后的 ROS 消息 dict 展平为「表名 → 行列表」。

        Args:
            msg: ``mcap_ros2`` 解码后的消息 dict（字段名与 .msg 定义一致）
            ctx: 管线注入的 ``msg_id``（本条消息序号）与 ``time_us``（微秒时间戳）

        Returns:
            键为 ``tables()`` 中的表名，值为行 dict 列表；每行键须覆盖该表全部列
            （缺失列可省略，插入时为 NULL）。主表通常 1 行/消息；数组子表 0~N 行。

        新 msg 类型：
        - 主表：``{"your_main": [_scalar_row(msg, ctx)]}``
        - 数组：``"your_items": _expand_array(msg_id, msg.get("items"), lambda x: {...})``
        - 可选 struct：判断 ``isinstance(x, dict)`` 后手工组 0/1 行（见 ``mem_detail_stat``）
        """
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
        # 单条嵌套 struct（非数组）：无 idx，每消息 0 或 1 行
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


# 模块 import 时注册到全局 registry；并在 adapters/__init__.py 中 import 本模块以触发注册
registry.register(SystemStatsAdapter())
