import request from '@/utils/request';
import type { Datasource, DatasourcePluginMeta } from '@/types';

export function getDatasourcePlugins() {
  return request<DatasourcePluginMeta[]>('/api/datasource/plugins');
}

export function getDatasources() {
  return request<Datasource[]>('/api/datasources');
}

export function getDatasource(id: number) {
  return request<Datasource>(`/api/datasources/${id}`);
}

export function createDatasource(data: Partial<Datasource>) {
  return request<Datasource>('/api/datasources', {
    method: 'POST',
    body: JSON.stringify(data),
  });
}

export function updateDatasource(id: number, data: Partial<Datasource>) {
  return request<Datasource>(`/api/datasources/${id}`, {
    method: 'PUT',
    body: JSON.stringify(data),
  });
}

export function deleteDatasource(id: number) {
  return request<string>(`/api/datasources/${id}`, { method: 'DELETE' });
}
