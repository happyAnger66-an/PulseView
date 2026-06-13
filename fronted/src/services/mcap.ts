import request from '@/utils/request';
import type { McapTopic } from '@/types';

/** 通用文件扫描；按 plugin_type 分派到对应格式的 inspect */
export function inspectSource(path: string, pluginType = 'ros2_mcap') {
  return request<{ topics: McapTopic[] }>(
    `/api/inspect?plugin_type=${encodeURIComponent(pluginType)}&path=${encodeURIComponent(path)}`,
  );
}

export function inspectMcap(path: string) {
  return inspectSource(path, 'ros2_mcap');
}
