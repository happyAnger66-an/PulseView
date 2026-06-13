import { Alert } from 'antd';
import SqlGraph from '@/components/SqlGraph';
import type { QueryPanelProps } from '@/types';

export default function Ros2McapQueryPanel({ datasource }: QueryPanelProps) {
  return (
    <>
      <Alert
        type='info'
        showIcon
        style={{ marginBottom: 16 }}
        message='ROS2 MCAP 数据源：MCAP 已导入 DuckDB，使用 SQL 查询。Ctrl+Enter 执行。'
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
