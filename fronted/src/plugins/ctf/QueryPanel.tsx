import { Alert } from 'antd';
import SqlGraph from '@/components/SqlGraph';
import type { QueryPanelProps } from '@/types';

export default function CtfQueryPanel({ datasource }: QueryPanelProps) {
  return (
    <>
      <Alert
        type='info'
        showIcon
        style={{ marginBottom: 16 }}
        message='CTF Trace 数据源：回调区间已转为 span 表，可用 Timeline 泳道视图查看。Ctrl+Enter 执行。'
      />
      <SqlGraph
        key={datasource.id}
        datasourceId={datasource.id}
        ingestStatus={datasource.ingest_status}
        ingestInfo={datasource.ingest_info}
      />
    </>
  );
}
