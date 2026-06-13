import { test, expect } from '@playwright/test';
import fs from 'fs';
import {
  createCtfDatasource,
  createProtobufDatasource,
  createRos2McapDatasource,
  getSchema,
  resetDatasources,
  runSqlQuery,
  seedReadyDatasource,
} from '../helpers/api';
import { TEST_CTF_PATH, TEST_MCAP_PATH, TEST_PROTO_PATH } from '../helpers/constants';

test.describe('Backend API', () => {
  test.beforeAll(() => {
    test.skip(!fs.existsSync(TEST_MCAP_PATH), `missing test mcap: ${TEST_MCAP_PATH}`);
  });

  test.beforeEach(async ({ request }) => {
    await resetDatasources(request);
  });

  test('lists datasource plugins', async ({ request }) => {
    const res = await request.get('/api/datasource/plugins');
    expect(res.ok()).toBeTruthy();
    const json = await res.json();
    const types = json.dat.map((p: { plugin_type: string }) => p.plugin_type);
    expect(types).toContain('ros2_mcap');
    expect(types).toContain('sqlite');
  });

  test('creates ros2 datasource and ingests mcap', async ({ request }) => {
    const ds = await createRos2McapDatasource(request);
    expect(ds.id).toBeGreaterThan(0);
    expect(ds.ingest_status).toBe('ready');
    expect(ds.settings['mcap.topic']).toBe('/slave/system_stats');
  });

  test('returns schema with dimension metadata', async ({ request }) => {
    const ds = await seedReadyDatasource(request);
    const schema = await getSchema(request, ds.id);
    const pubTable = schema.tables.find((t) => t.name === 'system_stats_node_pub_stats');
    expect(pubTable).toBeTruthy();
    expect(pubTable?.dimension_keys).toEqual(['node', 'topic']);
  });

  test('runs scalar sql query', async ({ request }) => {
    const ds = await seedReadyDatasource(request);
    const result = await runSqlQuery(
      request,
      ds.id,
      'SELECT _time, cpu_used_percent FROM system_stats ORDER BY _time',
    );
    expect(result.meta.time_column).toBe('_time');
    expect(result.meta.value_columns).toContain('cpu_used_percent');
    expect(result.meta.row_count).toBeGreaterThan(0);
  });

  test('runs dimension sql query for node sub hz', async ({ request }) => {
    const ds = await seedReadyDatasource(request);
    const result = await runSqlQuery(
      request,
      ds.id,
      `SELECT s._time, p.node, p.topic, p.hz
       FROM system_stats s
       JOIN system_stats_node_sub_stats p ON p.msg_id = s.msg_id
       ORDER BY s._time, p.node, p.topic`,
    );
    expect(result.meta.dimension_columns).toEqual(['node', 'topic']);
    expect(result.meta.value_columns).toEqual(['hz']);
    expect(result.meta.row_count).toBeGreaterThan(0);
  });

  test('rejects non-select sql', async ({ request }) => {
    const ds = await seedReadyDatasource(request);
    const res = await request.post('/api/sql/query', {
      data: { datasource_id: ds.id, sql: 'DELETE FROM system_stats' },
    });
    expect(res.status()).toBe(400);
  });

  test('ingests protobuf datasource and queries metrics', async ({ request }) => {
    test.skip(!fs.existsSync(TEST_PROTO_PATH), `missing test proto: ${TEST_PROTO_PATH}`);
    await resetDatasources(request);
    const ds = await createProtobufDatasource(request);
    expect(ds.ingest_status).toBe('ready');

    const schema = await getSchema(request, ds.id);
    const coresTable = schema.tables.find((t) => t.name === 'proto_metric_cores');
    expect(coresTable?.dimension_keys).toEqual(['core']);

    const result = await runSqlQuery(
      request,
      ds.id,
      'SELECT _time, cpu_percent, mem_percent FROM proto_metric ORDER BY _time',
    );
    expect(result.meta.time_column).toBe('_time');
    expect(result.meta.value_columns).toContain('cpu_percent');
    expect(result.meta.row_count).toBeGreaterThan(0);
  });

  test('ingests ctf trace and queries spans', async ({ request }) => {
    test.skip(!fs.existsSync(TEST_CTF_PATH), `missing test ctf: ${TEST_CTF_PATH}`);
    await resetDatasources(request);
    const ds = await createCtfDatasource(request);
    expect(ds.ingest_status).toBe('ready');

    const schema = await getSchema(request, ds.id);
    const spans = schema.tables.find((t) => t.name === 'ctf_spans');
    expect(spans?.table_kind).toBe('span');

    const result = await runSqlQuery(
      request,
      ds.id,
      'SELECT _time, _dur, track, name FROM ctf_spans ORDER BY _time',
    );
    expect(result.meta.time_column).toBe('_time');
    expect(result.meta.dur_column).toBe('_dur');
    expect(result.meta.dimension_columns).toContain('track');
    expect(result.meta.row_count).toBeGreaterThan(0);
  });
});
