import app.ingest.importers  # noqa: F401 — ensure importers registered
from app.duckdb_engine import get_schema, run_query
from app.ingest.importer import format_registry
from app.ingest.proto_schema import MSG_TYPE, encode_delimited, get_message_class


def _write_sample_pb(path, count=10, cores=3):
    MetricSample = get_message_class()
    with path.open("wb") as f:
        for i in range(count):
            msg = MetricSample()
            msg.time_us = 1_700_000_000_000_000 + i * 1_000_000
            msg.host = "host-a"
            msg.cpu_percent = 10.0 + i
            msg.mem_percent = 50.0 + i
            for c in range(cores):
                core = msg.cores.add()
                core.core = f"cpu{c}"
                core.usage = float(c * 10 + i)
            f.write(encode_delimited(msg))


def test_proto_importer_registered():
    assert format_registry.has("protobuf")
    importer = format_registry.get("protobuf")
    assert "proto.path" in importer.required_settings()


def test_proto_inspect_counts_messages(tmp_path):
    pb = tmp_path / "s.pb"
    _write_sample_pb(pb, count=7)
    info = format_registry.get("protobuf").inspect(str(pb))
    topic = info["topics"][0]
    assert topic["msg_type"] == MSG_TYPE
    assert topic["message_count"] == 7


def test_proto_ingest_schema_and_query(tmp_path):
    pb = tmp_path / "s.pb"
    _write_sample_pb(pb, count=10, cores=3)
    db = tmp_path / "out.duckdb"

    importer = format_registry.get("protobuf")
    report = importer.ingest(db, {"proto.path": str(pb), "proto.msg_type": MSG_TYPE})
    assert report["messages_decoded"] == 10

    # schema 元数据由 _pv_table_meta 驱动
    schema = get_schema(db)
    tables = {t["name"]: t for t in schema["tables"]}
    assert "proto_metric" in tables
    cores = tables["proto_metric_cores"]
    assert cores["parent_table"] == "proto_metric"
    assert cores["dimension_keys"] == ["core"]

    # 主表标量查询
    main = run_query(db, "SELECT _time, cpu_percent FROM proto_metric ORDER BY _time")
    assert main["meta"]["value_columns"] == ["cpu_percent"]
    assert main["meta"]["row_count"] == 10

    # 子表多线维度查询
    sub = run_query(
        db,
        "SELECT s._time, c.core, c.usage FROM proto_metric s "
        "JOIN proto_metric_cores c ON c.msg_id = s.msg_id ORDER BY s._time, c.core",
    )
    assert sub["meta"]["dimension_columns"] == ["core"]
    assert sub["meta"]["value_columns"] == ["usage"]
    assert sub["meta"]["row_count"] == 30
