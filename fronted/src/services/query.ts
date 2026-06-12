import request from '@/utils/request';
import type { PromQueryRangeParams, PromQueryRangeResponse } from '@/types';

export function queryRange(params: PromQueryRangeParams) {
  return request<PromQueryRangeResponse>('/api/query_range', {
    method: 'POST',
    body: JSON.stringify(params),
  });
}

export function getLabelValues(label: string) {
  return request<string[]>(`/api/label/${label}/values`);
}

export function getLabels() {
  return request<string[]>('/api/labels');
}
