import type { APIRequestContext } from '@playwright/test';
import { API_BASE, TEST_MCAP_PATH, TEST_MSG_TYPE, TEST_TOPIC } from './constants';

interface ApiEnvelope<T> {
  dat: T;
  err: string;
}

export interface Datasource {
  id: number;
  name: string;
  plugin_type: string;
  ingest_status?: string;
  settings: Record<string, string>;
}

export interface SqlQueryResult {
  columns: string[];
  rows: unknown[][];
  meta: {
    time_column?: string;
    dimension_columns?: string[];
    value_columns?: string[];
    row_count: number;
  };
}

async function parseApi<T>(res: Awaited<ReturnType<APIRequestContext['get']>>): Promise<T> {
  const json = (await res.json()) as ApiEnvelope<T>;
  if (!res.ok() || json.err) {
    throw new Error(json.err || `API ${res.status()} ${res.url()}`);
  }
  return json.dat;
}

export async function listDatasources(request: APIRequestContext) {
  return parseApi<Datasource[]>(await request.get(`${API_BASE}/api/datasources`));
}

export async function deleteDatasource(request: APIRequestContext, id: number) {
  return parseApi<unknown>(await request.delete(`${API_BASE}/api/datasources/${id}`));
}

export async function createRos2McapDatasource(request: APIRequestContext, name = 'e2e-test') {
  return parseApi<Datasource>(
    await request.post(`${API_BASE}/api/datasources`, {
      data: {
        name,
        description: 'e2e fixture',
        plugin_type: 'ros2_mcap',
        settings: {
          'mcap.path': TEST_MCAP_PATH,
          'mcap.topic': TEST_TOPIC,
          'mcap.msg_type': TEST_MSG_TYPE,
        },
      },
    }),
  );
}

export async function resetDatasources(request: APIRequestContext) {
  const items = await listDatasources(request);
  for (const item of items) {
    await deleteDatasource(request, item.id);
  }
}

export async function getSchema(request: APIRequestContext, dsId: number) {
  return parseApi<{ tables: Array<{ name: string; dimension_keys?: string[] }> }>(
    await request.get(`${API_BASE}/api/datasources/${dsId}/schema`),
  );
}

export async function runSqlQuery(request: APIRequestContext, dsId: number, sql: string) {
  return parseApi<SqlQueryResult>(
    await request.post(`${API_BASE}/api/sql/query`, {
      data: { datasource_id: dsId, sql, limit: 1000 },
    }),
  );
}

export async function seedReadyDatasource(request: APIRequestContext) {
  await resetDatasources(request);
  const ds = await createRos2McapDatasource(request);
  if (ds.ingest_status !== 'ready') {
    throw new Error(`ingest not ready: ${ds.ingest_status}`);
  }
  return ds;
}
