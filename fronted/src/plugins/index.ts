import { DatasourceForm as SqliteForm, PLUGIN_TYPE as SQLITE_TYPE, PLUGIN_NAME as SQLITE_NAME } from './sqlite';
import { DatasourceForm as Ros2McapForm, PLUGIN_TYPE as ROS2_TYPE, PLUGIN_NAME as ROS2_NAME } from './ros2_mcap';
import type { PluginDefinition, QueryLanguage } from '@/types';

export const PLUGINS: Record<string, PluginDefinition> = {
  [SQLITE_TYPE]: {
    type: SQLITE_TYPE,
    name: SQLITE_NAME,
    queryLanguage: 'promql',
    DatasourceForm: SqliteForm,
  },
  [ROS2_TYPE]: {
    type: ROS2_TYPE,
    name: ROS2_NAME,
    queryLanguage: 'sql',
    DatasourceForm: Ros2McapForm,
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
