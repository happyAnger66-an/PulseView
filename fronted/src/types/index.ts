import type { ComponentType } from 'react';

export type QueryLanguage = 'promql' | 'sql';

export interface SqliteSettings {
  'sqlite.path': string;
}

export interface Ros2McapSettings {
  'mcap.path': string;
  'mcap.topic': string;
  'mcap.msg_type': string;
}

export type DatasourceSettings = SqliteSettings | Ros2McapSettings;

export interface Datasource {
  id: number;
  name: string;
  description?: string;
  plugin_type: 'sqlite' | 'ros2_mcap';
  plugin_type_name: string;
  settings: DatasourceSettings;
  is_default: boolean;
  created_at?: number;
  updated_at?: number;
  ingest_status?: 'pending' | 'running' | 'ready' | 'error';
  ingest_info?: Record<string, unknown>;
}

export interface DatasourcePluginMeta {
  plugin_type: string;
  plugin_type_name: string;
  category: string;
}

export interface PromQueryRangeParams {
  datasource_id: number;
  query: string;
  start: number;
  end: number;
  step: number;
}

export interface PromMatrixResult {
  metric: Record<string, string>;
  values: [number, string][];
}

export interface PromQueryRangeResponse {
  status: string;
  data: {
    resultType: string;
    result: PromMatrixResult[];
  };
}

export interface McapTopic {
  name: string;
  msg_type: string | null;
  message_count: number;
}

export interface SchemaTable {
  name: string;
  columns: { name: string; type: string }[];
  parent_table?: string;
  join_key?: string;
  dimension_keys?: string[];
  default_metrics?: string[];
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

export interface PluginDefinition {
  type: string;
  name: string;
  queryLanguage: QueryLanguage;
  DatasourceForm: ComponentType<DatasourceFormProps>;
}

export interface DatasourceFormProps {
  action: 'add' | 'edit';
  data?: Partial<Datasource>;
  onFinish: (values: Pick<Datasource, 'name' | 'description' | 'settings'>) => void;
  submitLoading?: boolean;
}
