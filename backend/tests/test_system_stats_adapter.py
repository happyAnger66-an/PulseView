from app.ingest.adapters.system_stats import SystemStatsAdapter


def test_system_stats_tables_have_dimension_metadata():
    adapter = SystemStatsAdapter()
    tables = {t.name: t for t in adapter.tables()}

    pub = tables["system_stats_node_pub_stats"]
    assert pub.parent_table == "system_stats"
    assert pub.dimension_keys == ["node", "topic"]
    assert "hz" in pub.default_metrics

    main = tables["system_stats"]
    assert "cpu_used_percent" in main.default_metrics


def test_system_stats_flatten_expands_nested_arrays():
    adapter = SystemStatsAdapter()
    ctx = type("Ctx", (), {"msg_id": 1, "time_us": 1_700_000_000_000_000})()
    rows = adapter.flatten(
        {
            "header": {"stamp": {"sec": 1, "nanosec": 0}, "frame_id": "", "seq": 0},
            "hostname": "host-a",
            "cpu_used_percent": 12.5,
            "mem_used_percent": 40.0,
            "node_pub_stats": [
                {"node": "/n1", "topic": "/t1", "hz": 10.0},
                {"node": "/n2", "topic": "/t2", "hz": 20.0},
            ],
        },
        ctx,
    )

    assert len(rows["system_stats"]) == 1
    assert rows["system_stats"][0]["cpu_used_percent"] == 12.5
    assert len(rows["system_stats_node_pub_stats"]) == 2
    assert rows["system_stats_node_pub_stats"][0]["node"] == "/n1"
    assert rows["system_stats_node_pub_stats"][1]["hz"] == 20.0
