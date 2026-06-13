import {
  DatasourceForm as SqliteForm,
  QueryPanel as SqliteQueryPanel,
  PLUGIN_TYPE as SQLITE_TYPE,
  PLUGIN_NAME as SQLITE_NAME,
} from './sqlite';
import {
  DatasourceForm as Ros2McapForm,
  QueryPanel as Ros2McapQueryPanel,
  PLUGIN_TYPE as ROS2_TYPE,
  PLUGIN_NAME as ROS2_NAME,
} from './ros2_mcap';
import {
  DatasourceForm as ProtobufForm,
  QueryPanel as ProtobufQueryPanel,
  PLUGIN_TYPE as PROTOBUF_TYPE,
  PLUGIN_NAME as PROTOBUF_NAME,
} from './protobuf';
import {
  DatasourceForm as CtfForm,
  QueryPanel as CtfQueryPanel,
  PLUGIN_TYPE as CTF_TYPE,
  PLUGIN_NAME as CTF_NAME,
} from './ctf';
import {
  DatasourceForm as PerfettoForm,
  QueryPanel as PerfettoQueryPanel,
  PLUGIN_TYPE as PERFETTO_TYPE,
  PLUGIN_NAME as PERFETTO_NAME,
} from './perfetto';
import type { PluginDefinition, QueryLanguage } from '@/types';

export const PLUGINS: Record<string, PluginDefinition> = {
  [SQLITE_TYPE]: {
    type: SQLITE_TYPE,
    name: SQLITE_NAME,
    queryLanguage: 'promql',
    capabilities: ['promql'],
    defaultVisualizations: ['timeseries', 'table'],
    DatasourceForm: SqliteForm,
    QueryPanel: SqliteQueryPanel,
  },
  [ROS2_TYPE]: {
    type: ROS2_TYPE,
    name: ROS2_NAME,
    queryLanguage: 'sql',
    capabilities: ['ingest', 'schema', 'sql'],
    defaultVisualizations: ['timeseries', 'table'],
    DatasourceForm: Ros2McapForm,
    QueryPanel: Ros2McapQueryPanel,
  },
  [PROTOBUF_TYPE]: {
    type: PROTOBUF_TYPE,
    name: PROTOBUF_NAME,
    queryLanguage: 'sql',
    capabilities: ['ingest', 'schema', 'sql'],
    defaultVisualizations: ['timeseries', 'table'],
    DatasourceForm: ProtobufForm,
    QueryPanel: ProtobufQueryPanel,
  },
  [CTF_TYPE]: {
    type: CTF_TYPE,
    name: CTF_NAME,
    queryLanguage: 'sql',
    capabilities: ['ingest', 'schema', 'sql'],
    defaultVisualizations: ['timeline', 'table'],
    DatasourceForm: CtfForm,
    QueryPanel: CtfQueryPanel,
  },
  [PERFETTO_TYPE]: {
    type: PERFETTO_TYPE,
    name: PERFETTO_NAME,
    queryLanguage: 'sql',
    capabilities: ['ingest', 'schema', 'sql'],
    defaultVisualizations: ['timeline', 'table'],
    DatasourceForm: PerfettoForm,
    QueryPanel: PerfettoQueryPanel,
  },
};

export function getPlugin(type: string): PluginDefinition | undefined {
  return PLUGINS[type];
}

export function getPluginList(): PluginDefinition[] {
  return Object.values(PLUGINS);
}

export function getQueryLanguage(pluginType: string): QueryLanguage {
  return PLUGINS[pluginType]?.queryLanguage ?? 'promql';
}
