import { useMemo, useEffect, useRef, useState, useCallback } from 'react';
import UPlotReact from 'uplot-react';
import uPlot from 'uplot';
import 'uplot/dist/uPlot.min.css';

const SERIES_COLORS = ['#6C53B1', '#1890ff', '#52c41a', '#fa8c16', '#eb2f96', '#13c2c2'];

interface Props {
  columns: string[];
  rows: unknown[][];
  timeColumn?: string;
  valueColumns?: string[];
  height?: number;
}

function parseTime(value: unknown): number | null {
  if (value == null) return null;
  const n = Number(value);
  if (Number.isNaN(n)) return null;
  // microseconds → seconds
  if (n > 1e12) return n / 1_000_000;
  return n;
}

function buildChartData(
  columns: string[],
  rows: unknown[][],
  timeColumn: string,
  valueColumns: string[],
) {
  const timeIdx = columns.indexOf(timeColumn);
  const valueIdxs = valueColumns.map((col) => columns.indexOf(col));

  const points = rows
    .map((row) => {
      const t = parseTime(row[timeIdx]);
      if (t == null) return null;
      const values = valueIdxs.map((idx) => {
        if (idx < 0) return null;
        const v = row[idx];
        if (v == null) return null;
        const n = Number(v);
        return Number.isNaN(n) ? null : n;
      });
      if (values.every((v) => v == null)) return null;
      return { t, values };
    })
    .filter((p): p is { t: number; values: (number | null)[] } => p != null)
    .sort((a, b) => a.t - b.t);

  const xData = points.map((p) => p.t);
  const series: uPlot.Series[] = [{}];
  const alignedData: uPlot.AlignedData = [xData];

  valueColumns.forEach((col, i) => {
    series.push({
      label: col,
      stroke: SERIES_COLORS[i % SERIES_COLORS.length],
      width: 2,
      spanGaps: true,
    });
    alignedData.push(points.map((p) => p.values[i] ?? null));
  });

  return { alignedData, series, pointCount: xData.length };
}

export default function SqlTimeseriesChart({
  columns,
  rows,
  timeColumn,
  valueColumns,
  height = 320,
}: Props) {
  const containerRef = useRef<HTMLDivElement>(null);
  const [width, setWidth] = useState(0);

  const measure = useCallback(() => {
    if (!containerRef.current) return;
    const w = containerRef.current.getBoundingClientRect().width;
    if (w > 0) setWidth(Math.max(300, Math.floor(w)));
  }, []);

  useEffect(() => {
    measure();
    if (!containerRef.current) return;
    const ro = new ResizeObserver(() => measure());
    ro.observe(containerRef.current);
    return () => ro.disconnect();
  }, [measure, columns, rows]);

  const chartData = useMemo(() => {
    if (!timeColumn || !valueColumns?.length) return null;
    return buildChartData(columns, rows, timeColumn, valueColumns);
  }, [columns, rows, timeColumn, valueColumns]);

  const options = useMemo((): uPlot.Options | null => {
    if (!chartData) return null;
    return {
      width: width || 600,
      height,
      series: chartData.series,
      scales: { x: { time: true }, y: { auto: true } },
      axes: [
        { stroke: '#888', grid: { show: true } },
        { stroke: '#888', grid: { show: true } },
      ],
      legend: { show: true },
      cursor: { show: true },
    };
  }, [chartData, width, height]);

  if (!chartData || chartData.pointCount === 0) {
    return (
      <div className='chart-empty' ref={containerRef}>
        当前结果无法绘制时序图（需包含 _time 与数值列）
      </div>
    );
  }

  if (!options || width === 0) {
    return <div className='timeseries-chart' ref={containerRef} style={{ minHeight: height }} />;
  }

  return (
    <div className='timeseries-chart' ref={containerRef} style={{ minHeight: height }}>
      <UPlotReact
        key={`${width}-${chartData.pointCount}-${valueColumns?.join(',')}`}
        options={options}
        data={chartData.alignedData}
      />
    </div>
  );
}
