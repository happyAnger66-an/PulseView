import { useMemo, useEffect, useRef, useState } from 'react';
import UPlotReact from 'uplot-react';
import uPlot from 'uplot';
import 'uplot/dist/uPlot.min.css';
import type { PromMatrixResult } from '@/types';

interface Props {
  data: PromMatrixResult[];
  height?: number;
}

function buildSeries(data: PromMatrixResult[]) {
  const timestamps = new Set<number>();
  data.forEach((s) => s.values.forEach(([t]) => timestamps.add(t)));
  const sortedTs = Array.from(timestamps).sort((a, b) => a - b);
  const xData = sortedTs.map((t) => t);

  const series: uPlot.Series[] = [{}];
  const alignedData: uPlot.AlignedData = [xData];

  data.forEach((item) => {
    const name =
      item.metric.__name__ +
      (Object.keys(item.metric).length > 1
        ? `{${Object.entries(item.metric)
            .filter(([k]) => k !== '__name__')
            .map(([k, v]) => `${k}="${v}"`)
            .join(',')}}`
        : '');
    series.push({ label: name });
    const valueMap = new Map(item.values.map(([t, v]) => [t, Number(v)]));
    alignedData.push(sortedTs.map((t) => valueMap.get(t) ?? null));
  });

  return { alignedData, series };
}

export default function TimeseriesChart({ data, height = 320 }: Props) {
  const containerRef = useRef<HTMLDivElement>(null);
  const [width, setWidth] = useState(800);
  const { alignedData, series } = useMemo(() => buildSeries(data), [data]);

  useEffect(() => {
    if (!containerRef.current) return;
    const ro = new ResizeObserver(([entry]) => {
      setWidth(Math.max(300, entry.contentRect.width));
    });
    ro.observe(containerRef.current);
    return () => ro.disconnect();
  }, []);

  if (!data.length) {
    return <div className='chart-empty'>暂无数据，请输入 PromQL 并点击查询</div>;
  }

  const options: uPlot.Options = {
    width,
    height,
    series,
    scales: {
      x: { time: true },
      y: { auto: true },
    },
    axes: [
      { stroke: '#888', grid: { show: true } },
      { stroke: '#888', grid: { show: true } },
    ],
    legend: { show: true },
  };

  return (
    <div className='timeseries-chart' ref={containerRef}>
      <UPlotReact options={options} data={alignedData} />
    </div>
  );
}
