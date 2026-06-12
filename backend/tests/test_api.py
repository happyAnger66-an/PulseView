def test_create_ros2_datasource_and_query(client, mcap_path):
    res = client.post(
        "/api/datasources",
        json={
            "name": "pytest-ds",
            "plugin_type": "ros2_mcap",
            "settings": {
                "mcap.path": str(mcap_path),
                "mcap.topic": "/slave/system_stats",
                "mcap.msg_type": "system_stats_interfaces/msg/SystemStats",
            },
        },
    )
    assert res.status_code == 200
    ds = res.json()["dat"]
    assert ds["ingest_status"] == "ready"

    schema = client.get(f"/api/datasources/{ds['id']}/schema").json()["dat"]
    table_names = [t["name"] for t in schema["tables"]]
    assert "system_stats" in table_names
    assert "system_stats_node_pub_stats" in table_names

    query = client.post(
        "/api/sql/query",
        json={
            "datasource_id": ds["id"],
            "sql": "SELECT _time, cpu_used_percent FROM system_stats ORDER BY _time",
        },
    )
    assert query.status_code == 200
    body = query.json()["dat"]
    assert body["meta"]["row_count"] > 0
    assert body["meta"]["value_columns"] == ["cpu_used_percent"]
