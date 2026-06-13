import { useEffect, useMemo, useRef, useState } from 'react';
import { Button, Empty, Space } from 'antd';
import type { VizProps } from './types';

interface Span {
  start: number;
  dur: number;
  track: string;
  label: string;
}

const PALETTE = [
  '#5B8FF9', '#61DDAA', '#65789B', '#F6BD16', '#7262FD',
  '#78D3F8', '#9661BC', '#F6903D', '#008685', '#F08BB4',
];

const LANE_H = 26;
const LANE_GAP = 4;
const GUTTER = 120; // 左侧泳道名宽度
const AXIS_H = 22;
const PAD_TOP = 8;

function colorFor(key: string): string {
  let h = 0;
  for (let i = 0; i < key.length; i += 1) h = (h * 31 + key.charCodeAt(i)) >>> 0;
  return PALETTE[h % PALETTE.length];
}

/** 把相对纳秒格式化为可读时间 */
function fmt(ns: number): string {
  const abs = Math.abs(ns);
  if (abs >= 1e9) return `${(ns / 1e9).toFixed(3)} s`;
  if (abs >= 1e6) return `${(ns / 1e6).toFixed(3)} ms`;
  if (abs >= 1e3) return `${(ns / 1e3).toFixed(3)} µs`;
  return `${ns} ns`;
}

export default function TimelineViz({ columns, rows, meta }: VizProps) {
  const canvasRef = useRef<HTMLCanvasElement>(null);
  const wrapRef = useRef<HTMLDivElement>(null);
  const [width, setWidth] = useState(800);

  const { spans, lanes, domain } = useMemo(() => {
    const timeIdx = meta.time_column ? columns.indexOf(meta.time_column) : -1;
    const durIdx = meta.dur_column ? columns.indexOf(meta.dur_column) : -1;
    const trackCol = meta.dimension_columns?.[0];
    const trackIdx = trackCol ? columns.indexOf(trackCol) : -1;
    const nameIdx = columns.indexOf('name');

    const list: Span[] = [];
    if (timeIdx >= 0 && durIdx >= 0) {
      for (const row of rows) {
        const start = Number(row[timeIdx]);
        const dur = Number(row[durIdx]);
        if (!Number.isFinite(start) || !Number.isFinite(dur)) continue;
        const track = trackIdx >= 0 ? String(row[trackIdx] ?? 'default') : 'default';
        const label = nameIdx >= 0 ? String(row[nameIdx] ?? '') : track;
        list.push({ start, dur, track, label });
      }
    }
    const laneSet = Array.from(new Set(list.map((s) => s.track))).sort();
    let t0 = Infinity;
    let t1 = -Infinity;
    for (const s of list) {
      t0 = Math.min(t0, s.start);
      t1 = Math.max(t1, s.start + s.dur);
    }
    if (!Number.isFinite(t0)) {
      t0 = 0;
      t1 = 1;
    }
    if (t1 <= t0) t1 = t0 + 1;
    return { spans: list, lanes: laneSet, domain: [t0, t1] as [number, number] };
  }, [columns, rows, meta]);

  const [view, setView] = useState<[number, number]>(domain);
  useEffect(() => setView(domain), [domain]);

  const [drag, setDrag] = useState<{ x0: number; x1: number } | null>(null);
  const [hover, setHover] = useState<{ x: number; y: number; span: Span } | null>(null);

  useEffect(() => {
    const el = wrapRef.current;
    if (!el) return;
    const ro = new ResizeObserver((entries) => {
      const w = entries[0]?.contentRect.width ?? 800;
      setWidth(Math.max(360, Math.floor(w)));
    });
    ro.observe(el);
    return () => ro.disconnect();
  }, []);

  const plotW = width - GUTTER;
  const height = PAD_TOP + AXIS_H + lanes.length * (LANE_H + LANE_GAP) + 4;
  const [vs, ve] = view;
  const scaleX = (t: number) => GUTTER + ((t - vs) / (ve - vs)) * plotW;

  useEffect(() => {
    const cv = canvasRef.current;
    if (!cv) return;
    const dpr = window.devicePixelRatio || 1;
    cv.width = width * dpr;
    cv.height = height * dpr;
    cv.style.width = `${width}px`;
    cv.style.height = `${height}px`;
    const ctx = cv.getContext('2d');
    if (!ctx) return;
    ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
    ctx.clearRect(0, 0, width, height);
    ctx.font = '12px -apple-system, system-ui, sans-serif';
    ctx.textBaseline = 'middle';

    const laneTop = (i: number) => PAD_TOP + AXIS_H + i * (LANE_H + LANE_GAP);

    // 泳道背景 + 名称
    lanes.forEach((lane, i) => {
      const y = laneTop(i);
      ctx.fillStyle = i % 2 ? '#fafafa' : '#f3f4f6';
      ctx.fillRect(GUTTER, y, plotW, LANE_H);
      ctx.fillStyle = '#444';
      ctx.fillText(lane, 8, y + LANE_H / 2);
    });

    // 时间轴刻度
    ctx.strokeStyle = '#e0e0e0';
    ctx.fillStyle = '#999';
    const ticks = 6;
    for (let k = 0; k <= ticks; k += 1) {
      const t = vs + ((ve - vs) * k) / ticks;
      const x = scaleX(t);
      ctx.beginPath();
      ctx.moveTo(x, PAD_TOP + AXIS_H);
      ctx.lineTo(x, height);
      ctx.stroke();
      ctx.fillText(fmt(t - domain[0]), x + 2, PAD_TOP + AXIS_H / 2);
    }

    // spans
    const laneIndex = new Map(lanes.map((l, i) => [l, i]));
    for (const s of spans) {
      if (s.start + s.dur < vs || s.start > ve) continue;
      const i = laneIndex.get(s.track) ?? 0;
      const y = laneTop(i);
      const x = scaleX(s.start);
      const w = Math.max(1.5, scaleX(s.start + s.dur) - x);
      ctx.fillStyle = colorFor(s.label);
      ctx.fillRect(Math.max(GUTTER, x), y + 2, w, LANE_H - 4);
      if (w > 36) {
        ctx.fillStyle = '#1f1f1f';
        ctx.save();
        ctx.beginPath();
        ctx.rect(Math.max(GUTTER, x), y, w, LANE_H);
        ctx.clip();
        ctx.fillText(s.label, Math.max(GUTTER, x) + 4, y + LANE_H / 2);
        ctx.restore();
      }
    }

    // 框选高亮
    if (drag) {
      const x0 = Math.min(drag.x0, drag.x1);
      const x1 = Math.max(drag.x0, drag.x1);
      ctx.fillStyle = 'rgba(91,143,249,0.18)';
      ctx.fillRect(x0, PAD_TOP + AXIS_H, x1 - x0, height - PAD_TOP - AXIS_H);
      ctx.strokeStyle = '#5B8FF9';
      ctx.strokeRect(x0, PAD_TOP + AXIS_H, x1 - x0, height - PAD_TOP - AXIS_H);
    }
  }, [spans, lanes, view, width, height, drag, domain, plotW, vs, ve]);

  if (!meta.dur_column || spans.length === 0) {
    return <Empty description='无区间数据（需 _time + _dur 列）' />;
  }

  const pxToTime = (px: number) => vs + ((px - GUTTER) / plotW) * (ve - vs);

  const onDown = (e: React.MouseEvent) => {
    const rect = canvasRef.current!.getBoundingClientRect();
    const x = e.clientX - rect.left;
    if (x < GUTTER) return;
    setDrag({ x0: x, x1: x });
  };
  const onMove = (e: React.MouseEvent) => {
    const rect = canvasRef.current!.getBoundingClientRect();
    const x = e.clientX - rect.left;
    const y = e.clientY - rect.top;
    if (drag) {
      setDrag({ ...drag, x1: x });
      return;
    }
    // hover 命中检测
    const laneIndex = new Map(lanes.map((l, i) => [l, i]));
    let hit: Span | null = null;
    for (const s of spans) {
      const i = laneIndex.get(s.track) ?? 0;
      const ly = PAD_TOP + AXIS_H + i * (LANE_H + LANE_GAP);
      if (y < ly + 2 || y > ly + LANE_H - 2) continue;
      const sx = scaleX(s.start);
      const ex = scaleX(s.start + s.dur);
      if (x >= sx && x <= Math.max(ex, sx + 1.5)) {
        hit = s;
        break;
      }
    }
    setHover(hit ? { x, y, span: hit } : null);
  };
  const onUp = () => {
    if (drag) {
      const x0 = Math.min(drag.x0, drag.x1);
      const x1 = Math.max(drag.x0, drag.x1);
      if (x1 - x0 > 4) setView([pxToTime(x0), pxToTime(x1)]);
      setDrag(null);
    }
  };

  return (
    <div>
      <Space style={{ marginBottom: 8 }}>
        <Button size='small' onClick={() => setView(domain)}>
          重置缩放
        </Button>
        <span style={{ color: '#999', fontSize: 12 }}>
          {lanes.length} 泳道 / {spans.length} 区间 · 拖拽框选放大，双击重置
        </span>
      </Space>
      <div ref={wrapRef} style={{ position: 'relative', width: '100%', overflow: 'hidden' }}>
        <canvas
          ref={canvasRef}
          onMouseDown={onDown}
          onMouseMove={onMove}
          onMouseUp={onUp}
          onMouseLeave={() => {
            setHover(null);
            setDrag(null);
          }}
          onDoubleClick={() => setView(domain)}
          style={{ cursor: drag ? 'col-resize' : 'default', display: 'block' }}
        />
        {hover && (
          <div
            style={{
              position: 'absolute',
              left: Math.min(hover.x + 12, width - 200),
              top: hover.y + 12,
              background: 'rgba(0,0,0,0.82)',
              color: '#fff',
              padding: '6px 8px',
              borderRadius: 4,
              fontSize: 12,
              pointerEvents: 'none',
              whiteSpace: 'nowrap',
              zIndex: 10,
            }}
          >
            <div style={{ fontWeight: 600 }}>{hover.span.label}</div>
            <div>泳道: {hover.span.track}</div>
            <div>起点: {fmt(hover.span.start - domain[0])}</div>
            <div>时长: {fmt(hover.span.dur)}</div>
          </div>
        )}
      </div>
    </div>
  );
}
