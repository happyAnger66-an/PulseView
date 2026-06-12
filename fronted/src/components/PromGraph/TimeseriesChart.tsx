import { useMemo, useEffect, useRef, useState } from 'react';
import type { PromMatrixResult } from '@/types';
import { SERIES_COLORS } from '@/utils/timeseries';
import TimeseriesChartPanel from '@/components/TimeseriesChart';
import type uPlot from 'uplot';

interface Props {
  data: PromMatrixResult[];
  height?: number;
}

function buildSeries(data: PromMatrixResult[]) {
  const timestamps = new Set<number>();
  data.forEach((s) => s.values.forEach(([t]) => timestamps.add(t)));
  const sortedTs = Array.from(timestamps).sort((a, b) => a - b);

  const series: uPlot.Series[] = [{}];
  const alignedData: uPlot.AlignedData = [sortedTs];

  data.forEach((item, i) => {
    const name =
      item.metric.__name__ +
      (Object.keys(item.metric).length > 1
        ? `{${Object.entries(item.metric)
            .filter(([k]) => k !== '__name__')
            .map(([k, v]) => `${k}="${v}"`)
            .join(',')}}`
        : '');
    series.push({
      label: name,
      stroke: SERIES_COLORS[i % SERIES_COLORS.length],
      width: 2,
      spanGaps: true,
    });
    const valueMap = new Map(item.values.map(([t, v]) => [t, Number(v)]));
    alignedData.push(sortedTs.map((t) => valueMap.get(t) ?? null));
  });

  return { alignedData, series };
}

export default function TimeseriesChart({ data, height = 320 }: Props) {
  const containerRef = useRef<HTMLDivElement>(null);
  const [ready, setReady] = useState(false);
  const { alignedData, series } = useMemo(() => buildSeries(data), [data]);

  useEffect(() => {
    if (!containerRef.current) return;
    const ro = new ResizeObserver(([entry]) => {
      setReady(entry.contentRect.width > 0);
    });
    ro.observe(containerRef.current);
    return () => ro.disconnect();
  }, []);

  if (!data.length) {
    return <div className='chart-empty'>暂无数据，请输入 PromQL 并点击查询</div>;
  }

  if (!ready) {
    return <div className='timeseries-chart' ref={containerRef} style={{ minHeight: height }} />;
  }

  return (
    <div className='timeseries-chart' ref={containerRef}>
      <TimeseriesChartPanel alignedData={alignedData} series={series} height={height} />
    </div>
  );
}
