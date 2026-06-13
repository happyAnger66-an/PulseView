import { Alert } from 'antd';
import PromGraph from '@/components/PromGraph';
import type { QueryPanelProps } from '@/types';

export default function SqliteQueryPanel({ datasource }: QueryPanelProps) {
  return (
    <>
      <Alert
        type='info'
        showIcon
        style={{ marginBottom: 16 }}
        message='SQLite 数据源：通过 PromQL 查询时序指标。'
      />
      <PromGraph key={datasource.id} datasourceId={datasource.id} />
    </>
  );
}
