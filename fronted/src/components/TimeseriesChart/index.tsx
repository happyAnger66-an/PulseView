import { useMemo, useEffect, useRef, useState, useCallback } from 'react';
import { Button, Space } from 'antd';
import dayjs from 'dayjs';
import UPlotReact from 'uplot-react';
import uPlot from 'uplot';
import 'uplot/dist/uPlot.min.css';
import { SERIES_COLORS, yAxisValues } from '@/utils/timeseries';
import { statsFromAlignedData } from '@/utils/stats';
import SeriesStatsTable from './SeriesStatsTable';
import './style.less';

interface Props {
  alignedData: uPlot.AlignedData;
  series: uPlot.Series[];
  height?: number;
}

export default function TimeseriesChartPanel({ alignedData, series, height = 320 }: Props) {
  const wrapRef = useRef<HTMLDivElement>(null);
  const chartRef = useRef<uPlot | null>(null);
  const [width, setWidth] = useState(0);
  const [hidden, setHidden] = useState<Set<number>>(() => new Set());
  const [timeRange, setTimeRange] = useState<[number, number] | null>(null);

  const seriesKey = series
    .slice(1)
    .map((s) => s.label ?? '')
    .join('\0');

  const seriesLabels = useMemo(
    () => series.slice(1).map((s, i) => (typeof s.label === 'string' ? s.label : `series ${i + 1}`)),
    [series],
  );

  const legendItems = useMemo(
    () =>
      series.slice(1).map((s, i) => ({
        idx: i + 1,
        label: typeof s.label === 'string' ? s.label : `series ${i + 1}`,
        color: (s.stroke as string) || SERIES_COLORS[i % SERIES_COLORS.length],
      })),
    [series],
  );

  const multiSeries = legendItems.length > 1;

  const styledSeries = useMemo((): uPlot.Series[] => {
    if (!series.length) return series;
    const next = [...series];
    next[0] = { ...next[0] };
    for (let i = 1; i < next.length; i++) {
      next[i] = {
        ...next[i],
        stroke: next[i].stroke || SERIES_COLORS[(i - 1) % SERIES_COLORS.length],
        width: next[i].width ?? 2,
        spanGaps: next[i].spanGaps ?? true,
      };
    }
    return next;
  }, [series]);

  const statsRows = useMemo(
    () => statsFromAlignedData(alignedData, seriesLabels, hidden, timeRange),
    [alignedData, seriesLabels, hidden, timeRange],
  );

  useEffect(() => {
    setHidden(new Set());
    setTimeRange(null);
  }, [seriesKey]);

  const measure = useCallback(() => {
    if (!wrapRef.current) return;
    const legendWidth = multiSeries ? 280 : 0;
    const w = wrapRef.current.getBoundingClientRect().width - legendWidth - (multiSeries ? 12 : 0);
    if (w > 0) setWidth(Math.max(300, Math.floor(w)));
  }, [multiSeries]);

  useEffect(() => {
    measure();
    if (!wrapRef.current) return;
    const ro = new ResizeObserver(() => measure());
    ro.observe(wrapRef.current);
    return () => ro.disconnect();
  }, [measure, seriesKey]);

  const applyVisibility = useCallback((nextHidden: Set<number>) => {
    const chart = chartRef.current;
    if (!chart) return;
    for (let i = 1; i < chart.series.length; i++) {
      chart.setSeries(i, { show: !nextHidden.has(i) });
    }
  }, []);

  const toggleSeries = (idx: number) => {
    setHidden((prev) => {
      const next = new Set(prev);
      if (next.has(idx)) next.delete(idx);
      else next.add(idx);
      applyVisibility(next);
      return next;
    });
  };

  const showAll = () => {
    const next = new Set<number>();
    setHidden(next);
    applyVisibility(next);
  };

  const hideAll = () => {
    const next = new Set(legendItems.map((item) => item.idx));
    setHidden(next);
    applyVisibility(next);
  };

  const clearSelection = () => {
    setTimeRange(null);
    const chart = chartRef.current;
    if (chart) {
      chart.setSelect({ left: 0, top: 0, width: 0, height: 0 }, false);
    }
  };

  const handleSetSelect = useCallback((u: uPlot) => {
    if (u.select.width > 0) {
      const left = u.posToVal(u.select.left, 'x');
      const right = u.posToVal(u.select.left + u.select.width, 'x');
      setTimeRange([Math.min(left, right), Math.max(left, right)]);
    } else {
      setTimeRange(null);
    }
  }, []);

  const options = useMemo((): uPlot.Options => {
    return {
      width: width || 600,
      height,
      series: styledSeries,
      scales: { x: { time: true }, y: { auto: true } },
      axes: [
        { stroke: '#888', grid: { show: true } },
        { stroke: '#888', grid: { show: true }, values: yAxisValues },
      ],
      legend: { show: false },
      cursor: { show: true, focus: { prox: 16 } },
      select: { show: true, left: 0, top: 0, width: 0, height: 0 },
      hooks: { setSelect: [handleSetSelect] },
    };
  }, [styledSeries, width, height, handleSetSelect]);

  const handleCreate = useCallback((chart: uPlot) => {
    chartRef.current = chart;
  }, []);

  useEffect(() => {
    applyVisibility(hidden);
  }, [hidden, applyVisibility, width, seriesKey]);

  const rangeHint = timeRange
    ? `${dayjs.unix(timeRange[0]).format('HH:mm:ss')} — ${dayjs.unix(timeRange[1]).format('HH:mm:ss')}`
    : '全部数据';

  if (width === 0) {
    return <div className='timeseries-panel' ref={wrapRef} style={{ minHeight: height }} />;
  }

  return (
    <div className='timeseries-panel-wrap'>
      <div className={`timeseries-panel${multiSeries ? ' timeseries-panel--with-legend' : ''}`} ref={wrapRef}>
        <div className='timeseries-panel-chart' style={{ minHeight: height }}>
          <UPlotReact
            key={`${width}-${seriesKey}`}
            options={options}
            data={alignedData}
            onCreate={handleCreate}
          />
        </div>
        {multiSeries ? (
          <div className='timeseries-panel-legend' style={{ maxHeight: height }}>
            <Space size={4} className='timeseries-panel-legend-actions'>
              <Button size='small' type='link' onClick={showAll}>
                全部显示
              </Button>
              <Button size='small' type='link' onClick={hideAll}>
                全部隐藏
              </Button>
            </Space>
            <ul className='timeseries-legend-list'>
              {legendItems.map((item) => {
                const isHidden = hidden.has(item.idx);
                return (
                  <li key={item.idx}>
                    <button
                      type='button'
                      className={`timeseries-legend-item${isHidden ? ' is-hidden' : ''}`}
                      onClick={() => toggleSeries(item.idx)}
                      title={isHidden ? '点击显示' : '点击隐藏'}
                    >
                      <span className='timeseries-legend-marker' style={{ background: item.color }} />
                      <span className='timeseries-legend-label'>{String(item.label)}</span>
                    </button>
                  </li>
                );
              })}
            </ul>
          </div>
        ) : null}
      </div>
      <div className='timeseries-stats-section'>
        <div className='timeseries-stats-header'>
          <span className='timeseries-stats-title'>统计（{rangeHint}）</span>
          {timeRange ? (
            <Button size='small' type='link' onClick={clearSelection}>
              重置选区
            </Button>
          ) : (
            <span className='timeseries-stats-hint'>在图表上拖拽可选中时间范围</span>
          )}
        </div>
        <SeriesStatsTable rows={statsRows} />
      </div>
    </div>
  );
}
