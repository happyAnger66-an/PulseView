import SqlTimeseriesChart from '@/components/SqlGraph/SqlTimeseriesChart';
import type { VizProps } from './types';

export default function TimeseriesViz({ columns, rows, meta, height }: VizProps) {
  return (
    <SqlTimeseriesChart
      columns={columns}
      rows={rows}
      timeColumn={meta.time_column}
      dimensionColumns={meta.dimension_columns}
      valueColumns={meta.value_columns}
      height={height}
    />
  );
}
