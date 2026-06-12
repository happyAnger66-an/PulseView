import duckdb

from app.duckdb_engine import _infer_columns, run_query


def test_infer_columns_splits_dimensions_and_values():
    columns = ["_time", "node", "topic", "hz"]
    rows = [
        (1_000_000, "/a", "/cmd", 2.5),
        (2_000_000, "/b", "/odom", 3.1),
    ]
    time_col, dims, values = _infer_columns(columns, rows)
    assert time_col == "_time"
    assert dims == ["node", "topic"]
    assert values == ["hz"]


def test_infer_columns_excludes_timestamp_suffix_from_values():
    columns = ["_time", "node", "hz", "min_delta_ts"]
    rows = [(1, "/a", 2.5, 999999)]
    _, dims, values = _infer_columns(columns, rows)
    assert dims == ["node"]
    assert values == ["hz"]
    assert "min_delta_ts" not in values


def test_run_query_returns_dimension_metadata(tmp_path):
    db = tmp_path / "test.duckdb"
    conn = duckdb.connect(str(db))
    conn.execute("CREATE TABLE t (_time BIGINT, node VARCHAR, hz FLOAT)")
    conn.execute("INSERT INTO t VALUES (1000000, '/a', 2.5), (2000000, '/a', 3.0)")
    conn.close()

    result = run_query(
        db,
        "SELECT _time, node, hz FROM t ORDER BY _time",
    )
    assert result["meta"]["dimension_columns"] == ["node"]
    assert result["meta"]["value_columns"] == ["hz"]
    assert result["meta"]["row_count"] == 2
