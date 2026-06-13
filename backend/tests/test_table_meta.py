import duckdb

from app.duckdb_engine import get_schema
from app.ingest.importer import format_registry
from app.ingest.registry import ColumnDef, TableDef
from app.ingest.table_meta import META_TABLE, read_table_meta, write_table_meta


def _tabledefs():
    return [
        TableDef(
            name="main_t",
            columns=[ColumnDef("_time", "BIGINT"), ColumnDef("cpu", "FLOAT")],
            default_metrics=["cpu"],
        ),
        TableDef(
            name="sub_t",
            columns=[ColumnDef("msg_id", "BIGINT"), ColumnDef("node", "VARCHAR"), ColumnDef("hz", "FLOAT")],
            parent_table="main_t",
            dimension_keys=["node"],
            default_metrics=["hz"],
        ),
    ]


def test_write_and_read_table_meta(tmp_path):
    db = tmp_path / "m.duckdb"
    conn = duckdb.connect(str(db))
    write_table_meta(conn, _tabledefs())
    meta = read_table_meta(conn)
    conn.close()

    assert meta["sub_t"]["parent_table"] == "main_t"
    assert meta["sub_t"]["dimension_keys"] == ["node"]
    assert meta["sub_t"]["join_key"] == "msg_id"
    assert meta["main_t"]["table_kind"] == "timeseries"
    assert meta["main_t"]["default_metrics"] == ["cpu"]


def test_get_schema_uses_meta_and_excludes_meta_table(tmp_path):
    db = tmp_path / "s.duckdb"
    conn = duckdb.connect(str(db))
    conn.execute("CREATE TABLE main_t (_time BIGINT, cpu FLOAT)")
    conn.execute("CREATE TABLE sub_t (msg_id BIGINT, node VARCHAR, hz FLOAT)")
    write_table_meta(conn, _tabledefs())
    conn.close()

    schema = get_schema(db)
    names = [t["name"] for t in schema["tables"]]
    assert META_TABLE not in names
    sub = next(t for t in schema["tables"] if t["name"] == "sub_t")
    assert sub["parent_table"] == "main_t"
    assert sub["dimension_keys"] == ["node"]


def test_mcap_importer_registered():
    assert format_registry.has("ros2_mcap")
    importer = format_registry.get("ros2_mcap")
    assert "mcap.path" in importer.required_settings()
