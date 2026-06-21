import { useEffect, useMemo, useRef, useState, type CSSProperties } from 'react';
import { Button, Empty, Space, Alert } from 'antd';
import type { VizProps } from './types';

interface Span {
  start: number;
  dur: number;
  track: string;
  label: string;
  fields: Record<string, string | number>;
}

const PALETTE = [
  '#5B8FF9', '#61DDAA', '#65789B', '#F6BD16', '#7262FD',
  '#78D3F8', '#9661BC', '#F6903D', '#008685', '#F08BB4',
];

const FIELD_LABELS: Record<string, string> = {
  _time: '起点',
  _dur: '时长',
  track: '泳道',
  name: '名称',
  span_id: 'Span ID',
  cpu_id: 'TID',
  category: '分类',
  depth: '深度',
  callback: 'Callback',
};

const LANE_H = 26;
const LANE_GAP = 4;
const GUTTER = 120;
const AXIS_H = 22;
const PAD_TOP = 8;
const CLICK_DRAG_THRESHOLD = 4;
const PRIMARY_FIELDS = ['_time', '_dur', 'track', 'category', 'span_id', 'cpu_id', 'depth', 'callback'] as const;

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

function fieldLabel(name: string): string {
  return FIELD_LABELS[name] ?? name;
}

function formatFieldValue(name: string, value: string | number, timeBase: number): string {
  if (name === '_time') return fmt(Number(value) - timeBase);
  if (name === '_dur') return fmt(Number(value));
  return String(value);
}

function DetailPanel({
  span,
  timeBase,
  onClose,
}: {
  span: Span;
  timeBase: number;
  onClose: () => void;
}) {
  const name = span.fields.name != null ? String(span.fields.name) : span.label;
  const primary = PRIMARY_FIELDS.filter((k) => span.fields[k] != null).map((k) => ({
    key: k,
    label: fieldLabel(k),
    value: formatFieldValue(k, span.fields[k], timeBase),
  }));
  const seen = new Set<string>(['name', ...PRIMARY_FIELDS]);
  const extra = Object.keys(span.fields)
    .filter((k) => !seen.has(k))
    .map((k) => ({
      key: k,
      label: fieldLabel(k),
      value: formatFieldValue(k, span.fields[k], timeBase),
    }));

  const cellStyle: CSSProperties = {
    padding: '8px 12px',
    background: '#fff',
    border: '1px solid #f0f0f0',
    borderRadius: 6,
    minWidth: 0,
  };

  return (
    <div
      style={{
        marginTop: 12,
        border: '1px solid #e8e8e8',
        borderRadius: 8,
        background: '#fafafa',
        overflow: 'hidden',
      }}
    >
      <div
        style={{
          display: 'flex',
          alignItems: 'center',
          justifyContent: 'space-between',
          padding: '10px 14px',
          borderBottom: '1px solid #f0f0f0',
          background: '#fff',
        }}
      >
        <span style={{ fontWeight: 600, fontSize: 14 }}>区间详情</span>
        <Button size='small' type='text' onClick={onClose}>
          关闭
        </Button>
      </div>

      <div style={{ padding: 14, display: 'flex', flexDirection: 'column', gap: 12 }}>
        <div style={cellStyle}>
          <div style={{ fontSize: 12, color: '#8c8c8c', marginBottom: 6 }}>{fieldLabel('name')}</div>
          <div
            style={{
              fontFamily: 'Menlo, Monaco, Consolas, monospace',
              fontSize: 13,
              lineHeight: 1.55,
              wordBreak: 'break-word',
              whiteSpace: 'pre-wrap',
            }}
          >
            {name || '—'}
          </div>
        </div>

        {primary.length > 0 && (
          <div
            style={{
              display: 'grid',
              gridTemplateColumns: 'repeat(auto-fill, minmax(160px, 1fr))',
              gap: 10,
            }}
          >
            {primary.map((item) => (
              <div key={item.key} style={cellStyle}>
                <div style={{ fontSize: 12, color: '#8c8c8c', marginBottom: 4 }}>{item.label}</div>
                <div style={{ fontSize: 13, wordBreak: 'break-word' }}>{item.value}</div>
              </div>
            ))}
          </div>
        )}

        {extra.length > 0 && (
          <div
            style={{
              display: 'grid',
              gridTemplateColumns: 'repeat(auto-fill, minmax(200px, 1fr))',
              gap: 10,
            }}
          >
            {extra.map((item) => (
              <div key={item.key} style={cellStyle}>
                <div style={{ fontSize: 12, color: '#8c8c8c', marginBottom: 4 }}>{item.label}</div>
                <div style={{ fontSize: 13, wordBreak: 'break-word', whiteSpace: 'pre-wrap' }}>{item.value}</div>
              </div>
            ))}
          </div>
        )}
      </div>
    </div>
  );
}

function pickHitSpan(spans: Span[], lanes: string[], x: number, y: number, scaleX: (t: number) => number): Span | null {
  const laneIndex = new Map(lanes.map((l, i) => [l, i]));
  let best: { span: Span; area: number } | null = null;
  for (const s of spans) {
    const i = laneIndex.get(s.track) ?? 0;
    const ly = PAD_TOP + AXIS_H + i * (LANE_H + LANE_GAP);
    if (y < ly + 2 || y > ly + LANE_H - 2) continue;
    const sx = scaleX(s.start);
    const ex = scaleX(s.start + s.dur);
    if (x < sx || x > Math.max(ex, sx + 1.5)) continue;
    const area = Math.max(1, ex - sx) * LANE_H;
    if (!best || area < best.area) best = { span: s, area };
  }
  return best?.span ?? null;
}

export default function TimelineViz({ columns, rows, meta }: VizProps) {
  const canvasRef = useRef<HTMLCanvasElement>(null);
  const wrapRef = useRef<HTMLDivElement>(null);
  const [width, setWidth] = useState(800);

  const { spans, lanes, domain, timeBase } = useMemo(() => {
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
        const fields: Record<string, string | number> = {};
        columns.forEach((col, idx) => {
          const v = row[idx];
          if (v == null || v === '') return;
          fields[col] = typeof v === 'number' ? v : String(v);
        });
        list.push({ start, dur, track, label: label || track, fields });
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
    return { spans: list, lanes: laneSet, domain: [t0, t1] as [number, number], timeBase: t0 };
  }, [columns, rows, meta]);

  const [view, setView] = useState<[number, number]>(domain);
  useEffect(() => setView(domain), [domain]);

  const [drag, setDrag] = useState<{ x0: number; x1: number; y0: number } | null>(null);
  const [hover, setHover] = useState<{ x: number; y: number; span: Span } | null>(null);
  const [selected, setSelected] = useState<Span | null>(null);

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

    lanes.forEach((lane, i) => {
      const y = laneTop(i);
      ctx.fillStyle = i % 2 ? '#fafafa' : '#f3f4f6';
      ctx.fillRect(GUTTER, y, plotW, LANE_H);
      ctx.fillStyle = '#444';
      ctx.fillText(lane, 8, y + LANE_H / 2);
    });

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

    const laneIndex = new Map(lanes.map((l, i) => [l, i]));
    for (const s of spans) {
      if (s.start + s.dur < vs || s.start > ve) continue;
      const i = laneIndex.get(s.track) ?? 0;
      const y = laneTop(i);
      const x = scaleX(s.start);
      const w = Math.max(1.5, scaleX(s.start + s.dur) - x);
      const isSelected = selected === s;
      const isHover = hover?.span === s;
      ctx.fillStyle = colorFor(s.label);
      ctx.fillRect(Math.max(GUTTER, x), y + 2, w, LANE_H - 4);
      if (isSelected || isHover) {
        ctx.strokeStyle = isSelected ? '#1677ff' : '#69b1ff';
        ctx.lineWidth = isSelected ? 2 : 1.5;
        ctx.strokeRect(Math.max(GUTTER, x), y + 2, w, LANE_H - 4);
        ctx.lineWidth = 1;
      }
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

    if (drag) {
      const x0 = Math.min(drag.x0, drag.x1);
      const x1 = Math.max(drag.x0, drag.x1);
      ctx.fillStyle = 'rgba(91,143,249,0.18)';
      ctx.fillRect(x0, PAD_TOP + AXIS_H, x1 - x0, height - PAD_TOP - AXIS_H);
      ctx.strokeStyle = '#5B8FF9';
      ctx.strokeRect(x0, PAD_TOP + AXIS_H, x1 - x0, height - PAD_TOP - AXIS_H);
    }
  }, [spans, lanes, view, width, height, drag, domain, plotW, vs, ve, hover, selected]);

  if (!meta.dur_column || spans.length === 0) {
    return <Empty description='无区间数据（需 _time + _dur 列）' />;
  }

  const trackCol = meta.dimension_columns?.[0];
  const missingTrack = trackCol && !columns.includes(trackCol);

  const pxToTime = (px: number) => vs + ((px - GUTTER) / plotW) * (ve - vs);

  const pointerPos = (e: React.MouseEvent) => {
    const rect = canvasRef.current!.getBoundingClientRect();
    return { x: e.clientX - rect.left, y: e.clientY - rect.top };
  };

  const onDown = (e: React.MouseEvent) => {
    const { x, y } = pointerPos(e);
    if (x < GUTTER) return;
    setDrag({ x0: x, x1: x, y0: y });
  };

  const onMove = (e: React.MouseEvent) => {
    const { x, y } = pointerPos(e);
    if (drag) {
      setDrag({ ...drag, x1: x });
      return;
    }
    const hit = pickHitSpan(spans, lanes, x, y, scaleX);
    setHover(hit ? { x, y, span: hit } : null);
  };

  const onUp = (e: React.MouseEvent) => {
    if (!drag) return;
    const { x, y } = pointerPos(e);
    const dx = Math.abs(x - drag.x0);
    const dy = Math.abs(y - drag.y0);
    if (dx > CLICK_DRAG_THRESHOLD || dy > CLICK_DRAG_THRESHOLD) {
      const x0 = Math.min(drag.x0, drag.x1);
      const x1 = Math.max(drag.x0, drag.x1);
      if (x1 - x0 > 4) setView([pxToTime(x0), pxToTime(x1)]);
    } else {
      const hit = pickHitSpan(spans, lanes, x, y, scaleX);
      setSelected(hit);
    }
    setDrag(null);
  };

  return (
    <div>
      {missingTrack ? (
        <Alert
          type='info'
          showIcon
          style={{ marginBottom: 8 }}
          message={`当前查询未包含「${trackCol}」列，泳道无法分组。建议使用：SELECT _time, _dur, ${trackCol}, name, ... FROM ...`}
        />
      ) : null}
      <Space style={{ marginBottom: 8 }}>
        <Button size='small' onClick={() => setView(domain)}>
          重置缩放
        </Button>
        {selected ? (
          <Button size='small' onClick={() => setSelected(null)}>
            清除选中
          </Button>
        ) : null}
        <span style={{ color: '#999', fontSize: 12 }}>
          {lanes.length} 泳道 / {spans.length} 区间 · 点击区间查看详情 · 拖拽框选放大 · 双击重置
        </span>
      </Space>
      <div ref={wrapRef} style={{ width: '100%' }}>
        <div style={{ position: 'relative', width: '100%', overflow: 'hidden' }}>
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
            style={{ cursor: drag ? 'col-resize' : 'pointer', display: 'block', width: '100%' }}
          />
          {hover && !selected && (
            <div
              style={{
                position: 'absolute',
                left: Math.min(hover.x + 12, width - 240),
                top: hover.y + 12,
                background: 'rgba(0,0,0,0.82)',
                color: '#fff',
                padding: '6px 8px',
                borderRadius: 4,
                fontSize: 12,
                pointerEvents: 'none',
                maxWidth: Math.min(480, width - hover.x - 24),
                zIndex: 10,
              }}
            >
              <div style={{ fontWeight: 600, wordBreak: 'break-word' }}>{hover.span.label}</div>
              <div>时长: {fmt(hover.span.dur)}</div>
              <div>点击固定详情</div>
            </div>
          )}
        </div>
        {selected && (
          <DetailPanel span={selected} timeBase={timeBase} onClose={() => setSelected(null)} />
        )}
      </div>
    </div>
  );
}
