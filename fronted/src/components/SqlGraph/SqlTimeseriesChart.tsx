import { useMemo, useEffect, useRef, useState, useCallback } from 'react';
import { buildChartFromQueryResult } from '@/utils/timeseries';
import TimeseriesChartPanel from '@/components/TimeseriesChart';

interface Props {
  columns: string[];
  rows: unknown[][];
  timeColumn?: string;
  dimensionColumns?: string[];
  valueColumns?: string[];
  height?: number;
}

export default function SqlTimeseriesChart({
  columns,
  rows,
  timeColumn,
  dimensionColumns,
  valueColumns,
  height = 320,
}: Props) {
  const containerRef = useRef<HTMLDivElement>(null);
  const [ready, setReady] = useState(false);

  const measure = useCallback(() => {
    if (!containerRef.current) return;
    setReady(containerRef.current.getBoundingClientRect().width > 0);
  }, []);

  useEffect(() => {
    measure();
    if (!containerRef.current) return;
    const ro = new ResizeObserver(() => measure());
    ro.observe(containerRef.current);
    return () => ro.disconnect();
  }, [measure, columns, rows]);

  const chartData = useMemo(
    () => buildChartFromQueryResult(columns, rows, timeColumn, dimensionColumns, valueColumns),
    [columns, rows, timeColumn, dimensionColumns, valueColumns],
  );

  if (!chartData || chartData.pointCount === 0) {
    return (
      <div className='chart-empty' ref={containerRef}>
        当前结果无法绘制时序图（需包含 _time 与数值列）
      </div>
    );
  }

  if (!ready) {
    return <div className='timeseries-chart' ref={containerRef} style={{ minHeight: height }} />;
  }

  return (
    <div className='timeseries-chart' ref={containerRef} style={{ minHeight: height }}>
      <TimeseriesChartPanel
        alignedData={chartData.alignedData}
        series={chartData.series}
        height={height}
      />
    </div>
  );
}
