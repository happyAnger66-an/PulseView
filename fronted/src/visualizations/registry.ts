import type { VizDefinition, VizMeta } from './types';

const REGISTRY: Record<string, VizDefinition> = {};
const ORDER: string[] = [];

export function registerViz(def: VizDefinition): void {
  if (!REGISTRY[def.type]) ORDER.push(def.type);
  REGISTRY[def.type] = def;
}

export function getViz(type: string): VizDefinition | undefined {
  return REGISTRY[type];
}

export function getVizList(): VizDefinition[] {
  return ORDER.map((t) => REGISTRY[t]);
}

/** 返回适用于该结果的可视化列表，按注册顺序排列 */
export function selectViz(meta: VizMeta): VizDefinition[] {
  return getVizList().filter((v) => v.accepts(meta));
}
