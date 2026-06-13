import { registerViz, selectViz, getViz, getVizList } from './registry';
import TimeseriesViz from './TimeseriesViz';
import TimelineViz from './TimelineViz';
import TableViz from './TableViz';

registerViz({
  type: 'timeseries',
  name: '图表',
  component: TimeseriesViz,
  accepts: (meta) => Boolean(meta.time_column) && (meta.value_columns?.length ?? 0) > 0,
});

registerViz({
  type: 'timeline',
  name: 'Timeline',
  component: TimelineViz,
  accepts: (meta) => Boolean(meta.time_column) && Boolean(meta.dur_column),
});

registerViz({
  type: 'table',
  name: '表格',
  component: TableViz,
  accepts: () => true,
});

export { registerViz, selectViz, getViz, getVizList };
export type { VizDefinition, VizProps, VizMeta } from './types';
