import type uPlot from 'uplot';

export const SERIES_COLORS = ['#6C53B1', '#1890ff', '#52c41a', '#fa8c16', '#eb2f96', '#13c2c2'];

export interface TimeseriesSeries {
  metric: string;
  labels: Record<string, string>;
  values: [number, number | null][];
}

export function formatSeriesLabel(metric: string, labels: Record<string, string>): string {
  const entries = Object.entries(labels);
  if (!entries.length) return metric;
  const labelStr = entries.map(([k, v]) => `${k}="${v}"`).join(', ');
  return `${metric}{${labelStr}}`;
}

export function parseTime(value: unknown): number | null {
  if (value == null) return null;
  const n = Number(value);
  if (Number.isNaN(n)) return null;
  if (n > 1e12) return n / 1_000_000;
  return n;
}

/** Y 轴刻度格式化：按 tick 步长自适应小数位 */
export function formatAxisTick(value: number, tickIncr: number): string {
  if (!Number.isFinite(value)) return '';
  if (value === 0) return '0';

  const abs = Math.abs(value);
  const incr = Math.abs(tickIncr) || abs;

  if (abs >= 1e6 || (abs > 0 && abs < 1e-4)) {
    return value.toExponential(2);
  }

  let decimals = 0;
  if (incr < 1) {
    decimals = Math.min(4, Math.max(1, -Math.floor(Math.log10(incr))));
  } else if (incr < 10) {
    decimals = 1;
  }

  return value.toFixed(decimals);
}

export function yAxisValues(_u: uPlot, splits: number[], _idx: number, _space: number, incr: number) {
  return splits.map((v) => formatAxisTick(v, incr));
}

function labelKey(labels: Record<string, string>): string {
  return Object.entries(labels)
    .sort(([a], [b]) => a.localeCompare(b))
    .map(([k, v]) => `${k}=${v}`)
    .join('|');
}

/** long format → 按 dimension 分线的 series 列表 */
export function rowsToSeries(
  columns: string[],
  rows: unknown[][],
  timeColumn: string,
  dimensionColumns: string[],
  valueColumns: string[],
): TimeseriesSeries[] {
  const timeIdx = columns.indexOf(timeColumn);
  const dimIdxs = dimensionColumns.map((c) => columns.indexOf(c));
  const valIdxs = valueColumns.map((c) => columns.indexOf(c));

  const grouped = new Map<string, TimeseriesSeries>();

  for (const row of rows) {
    const t = parseTime(row[timeIdx]);
    if (t == null) continue;

    const labels: Record<string, string> = {};
    dimensionColumns.forEach((col, i) => {
      const idx = dimIdxs[i];
      if (idx >= 0 && row[idx] != null) {
        labels[col] = String(row[idx]);
      }
    });

    valueColumns.forEach((metric, vi) => {
      const vIdx = valIdxs[vi];
      if (vIdx < 0) return;
      const raw = row[vIdx];
      const val = raw == null ? null : Number(raw);
      const num = val == null || Number.isNaN(val) ? null : val;

      const key = `${metric}::${labelKey(labels)}`;
      let series = grouped.get(key);
      if (!series) {
        series = { metric, labels, values: [] };
        grouped.set(key, series);
      }
      series.values.push([t, num]);
    });
  }

  const result = Array.from(grouped.values());
  result.forEach((s) => s.values.sort((a, b) => a[0] - b[0]));
  return result;
}

/** 宽表：每列一条线（无 dimension） */
export function rowsToFlatSeries(
  columns: string[],
  rows: unknown[][],
  timeColumn: string,
  valueColumns: string[],
): TimeseriesSeries[] {
  const timeIdx = columns.indexOf(timeColumn);
  const valIdxs = valueColumns.map((c) => columns.indexOf(c));

  return valueColumns.map((metric, vi) => {
    const vIdx = valIdxs[vi];
    const values: [number, number | null][] = [];
    for (const row of rows) {
      const t = parseTime(row[timeIdx]);
      if (t == null) continue;
      const raw = row[vIdx];
      const val = raw == null ? null : Number(raw);
      values.push([t, val == null || Number.isNaN(val) ? null : val]);
    }
    values.sort((a, b) => a[0] - b[0]);
    return { metric, labels: {}, values };
  });
}

export function seriesToUPlot(seriesList: TimeseriesSeries[]): {
  alignedData: uPlot.AlignedData;
  series: uPlot.Series[];
  pointCount: number;
} {
  const timestamps = new Set<number>();
  seriesList.forEach((s) => s.values.forEach(([t]) => timestamps.add(t)));
  const sortedTs = Array.from(timestamps).sort((a, b) => a - b);

  const uplotSeries: uPlot.Series[] = [{}];
  const alignedData: uPlot.AlignedData = [sortedTs];

  seriesList.forEach((item, i) => {
    uplotSeries.push({
      label: formatSeriesLabel(item.metric, item.labels),
      stroke: SERIES_COLORS[i % SERIES_COLORS.length],
      width: 2,
      spanGaps: true,
    });
    const valueMap = new Map(item.values.map(([t, v]) => [t, v]));
    alignedData.push(sortedTs.map((t) => valueMap.get(t) ?? null));
  });

  return { alignedData, series: uplotSeries, pointCount: sortedTs.length };
}

export function buildChartFromQueryResult(
  columns: string[],
  rows: unknown[][],
  timeColumn?: string,
  dimensionColumns?: string[],
  valueColumns?: string[],
) {
  if (!timeColumn || !valueColumns?.length) return null;

  const hasDimensions = (dimensionColumns?.length ?? 0) > 0;
  const seriesList = hasDimensions
    ? rowsToSeries(columns, rows, timeColumn, dimensionColumns!, valueColumns)
    : rowsToFlatSeries(columns, rows, timeColumn, valueColumns);

  const nonEmpty = seriesList.filter((s) => s.values.some(([, v]) => v != null));
  if (!nonEmpty.length) return null;

  return seriesToUPlot(nonEmpty);
}
