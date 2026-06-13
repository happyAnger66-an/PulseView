import type { ComponentType } from 'react';
import type { SqlQueryResult } from '@/types';

export type VizMeta = SqlQueryResult['meta'];

export interface VizProps {
  columns: string[];
  rows: unknown[][];
  meta: VizMeta;
  height?: number;
}

export interface VizDefinition {
  /** 唯一标识，如 timeseries / table / timeline */
  type: string;
  /** tab 展示名 */
  name: string;
  component: ComponentType<VizProps>;
  /** 判断该查询结果是否适用此可视化 */
  accepts: (meta: VizMeta) => boolean;
}
