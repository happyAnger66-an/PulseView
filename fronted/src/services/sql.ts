import request from '@/utils/request';
import type { SchemaTable, SqlQueryResult } from '@/types';

export function getDatasourceSchema(datasourceId: number) {
  return request<{ tables: SchemaTable[] }>(`/api/datasources/${datasourceId}/schema`);
}

export function runSqlQuery(datasourceId: number, sql: string, limit = 1000) {
  return request<SqlQueryResult>('/api/sql/query', {
    method: 'POST',
    body: JSON.stringify({ datasource_id: datasourceId, sql, limit }),
  });
}

export function ingestDatasource(datasourceId: number, force = false) {
  return request<Record<string, unknown>>(`/api/datasources/${datasourceId}/ingest`, {
    method: 'POST',
    body: JSON.stringify({ force }),
  });
}
