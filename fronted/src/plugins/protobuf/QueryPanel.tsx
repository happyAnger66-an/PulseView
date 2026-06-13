import { Alert } from 'antd';
import SqlGraph from '@/components/SqlGraph';
import type { QueryPanelProps } from '@/types';

export default function ProtobufQueryPanel({ datasource }: QueryPanelProps) {
  return (
    <>
      <Alert
        type='info'
        showIcon
        style={{ marginBottom: 16 }}
        message='Protobuf 数据源：已导入 DuckDB，使用 SQL 查询。Ctrl+Enter 执行。'
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
