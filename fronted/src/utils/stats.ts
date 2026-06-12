import type uPlot from 'uplot';

export interface SeriesStats {
  label: string;
  count: number;
  avg: number | null;
  p50: number | null;
  p99: number | null;
}

function percentile(sorted: number[], p: number): number {
  if (sorted.length === 0) return NaN;
  if (sorted.length === 1) return sorted[0];
  const index = (p / 100) * (sorted.length - 1);
  const lower = Math.floor(index);
  const upper = Math.ceil(index);
  if (lower === upper) return sorted[lower];
  return sorted[lower] * (1 - (index - lower)) + sorted[upper] * (index - lower);
}

export function computeStats(values: number[]): Omit<SeriesStats, 'label'> {
  if (!values.length) {
    return { count: 0, avg: null, p50: null, p99: null };
  }
  const sorted = [...values].sort((a, b) => a - b);
  const sum = values.reduce((acc, v) => acc + v, 0);
  return {
    count: values.length,
    avg: sum / values.length,
    p50: percentile(sorted, 50),
    p99: percentile(sorted, 99),
  };
}

export function formatStatValue(value: number | null): string {
  if (value == null || !Number.isFinite(value)) return '-';
  const abs = Math.abs(value);
  if (abs >= 1e6 || (abs > 0 && abs < 1e-4)) return value.toExponential(3);
  if (abs >= 100) return value.toFixed(1);
  if (abs >= 1) return value.toFixed(3);
  return value.toFixed(4);
}

function findTimeRangeIndices(xs: number[], range: [number, number]): [number, number] {
  const [tMin, tMax] = range;
  let start = 0;
  while (start < xs.length && xs[start] < tMin) start += 1;
  let end = xs.length - 1;
  while (end >= 0 && xs[end] > tMax) end -= 1;
  if (start > end) return [-1, -1];
  return [start, end];
}

export function statsFromAlignedData(
  alignedData: uPlot.AlignedData,
  seriesLabels: string[],
  hiddenSeries: Set<number>,
  timeRange?: [number, number] | null,
): SeriesStats[] {
  const xs = alignedData[0] as number[];
  let iMin = 0;
  let iMax = xs.length - 1;

  if (timeRange) {
    [iMin, iMax] = findTimeRangeIndices(xs, timeRange);
    if (iMin < 0) return [];
  }

  const rows: SeriesStats[] = [];
  for (let si = 0; si < seriesLabels.length; si++) {
    const seriesIdx = si + 1;
    if (hiddenSeries.has(seriesIdx)) continue;

    const ys = alignedData[seriesIdx] as (number | null | undefined)[];
    const values: number[] = [];
    for (let i = iMin; i <= iMax; i++) {
      const v = ys[i];
      if (v != null && Number.isFinite(v)) values.push(v);
    }

    rows.push({
      label: seriesLabels[si],
      ...computeStats(values),
    });
  }
  return rows;
}
