import path from 'path';

const PULSEVIEW_ROOT = process.cwd();
export const TEST_MCAP_PATH =
  process.env.PULSEVIEW_TEST_MCAP ?? path.resolve(PULSEVIEW_ROOT, '../test2/test2_0.mcap');

export const TEST_TOPIC = '/slave/system_stats';
export const TEST_MSG_TYPE = 'system_stats_interfaces/msg/SystemStats';

export const API_BASE = process.env.PULSEVIEW_API_BASE ?? 'http://127.0.0.1:8080';
export const WEB_BASE = process.env.PULSEVIEW_WEB_BASE ?? 'http://127.0.0.1:8766';
