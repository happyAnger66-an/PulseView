import request from '@/utils/request';
import type { McapTopic } from '@/types';

export function inspectMcap(path: string) {
  return request<{ topics: McapTopic[] }>(`/api/mcap/inspect?path=${encodeURIComponent(path)}`);
}
