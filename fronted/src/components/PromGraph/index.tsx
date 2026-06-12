import { useState } from 'react';
import { Button, Tabs, Alert, Space, Select } from 'antd';
import dayjs from 'dayjs';
import PromQLInput from '@/components/PromQLInput';
import TimeseriesChart from './TimeseriesChart';
import ResultTable from './ResultTable';
import { queryRange } from '@/services/query';
import type { PromMatrixResult } from '@/types';
import './style.less';

const { TabPane } = Tabs;

interface Props {
  datasourceId: number;
  defaultQuery?: string;
}

const RANGE_PRESETS = [
  { label: '近 15 分钟', value: 15 * 60 },
  { label: '近 1 小时', value: 60 * 60 },
  { label: '近 6 小时', value: 6 * 60 * 60 },
  { label: '近 24 小时', value: 24 * 60 * 60 },
];

export default function PromGraph({ datasourceId, defaultQuery = 'cpu_usage' }: Props) {
  const [query, setQuery] = useState(defaultQuery);
  const [rangeSeconds, setRangeSeconds] = useState(RANGE_PRESETS[1].value);
  const [loading, setLoading] = useState(false);
  const [error, setError] = useState('');
  const [tab, setTab] = useState<'graph' | 'table'>('graph');
  const [result, setResult] = useState<PromMatrixResult[]>([]);

  const executeQuery = async (promql?: string) => {
    const q = promql ?? query;
    if (!q.trim()) return;
    setLoading(true);
    setError('');
    try {
      const end = Math.floor(dayjs().unix());
      const start = end - rangeSeconds;
      const res = await queryRange({
        datasource_id: datasourceId,
        query: q,
        start,
        end,
        step: Math.max(15, Math.floor(rangeSeconds / 250)),
      });
      setResult(res.data?.result ?? []);
      setQuery(q);
    } catch (e) {
      setError(e instanceof Error ? e.message : '查询失败');
      setResult([]);
    } finally {
      setLoading(false);
    }
  };

  return (
    <div className='prom-graph'>
      <div className='prom-graph-toolbar'>
        <Space wrap>
          <Select
            value={rangeSeconds}
            style={{ width: 140 }}
            options={RANGE_PRESETS}
            onChange={setRangeSeconds}
          />
          <span className='range-hint'>
            {dayjs.unix(Math.floor(dayjs().unix() - rangeSeconds)).format('HH:mm')} — {dayjs().format('HH:mm')}
          </span>
        </Space>
      </div>

      <div className='prom-graph-input-row'>
        <div className='prom-graph-input'>
          <PromQLInput
            value={query}
            onChange={(v) => setQuery(v ?? '')}
            onEnter={executeQuery}
          />
        </div>
        <Button type='primary' loading={loading} onClick={() => executeQuery()}>
          查询
        </Button>
      </div>

      {error && <Alert type='error' message={error} showIcon style={{ marginBottom: 12 }} />}

      <Tabs activeKey={tab} onChange={(k) => setTab(k as 'graph' | 'table')}>
        <TabPane tab='图表' key='graph'>
          <TimeseriesChart data={result} />
        </TabPane>
        <TabPane tab='表格' key='table'>
          <ResultTable data={result} />
        </TabPane>
      </Tabs>
    </div>
  );
}
