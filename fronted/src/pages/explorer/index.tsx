import { useEffect, useState } from 'react';
import { Card, Select, Empty, Alert } from 'antd';
import PromGraph from '@/components/PromGraph';
import SqlGraph from '@/components/SqlGraph';
import { useAppContext } from '@/components/Layout';
import { getQueryLanguage } from '@/plugins';

export default function ExplorerPage() {
  const { datasources, defaultDatasource } = useAppContext();
  const [datasourceId, setDatasourceId] = useState<number>();

  useEffect(() => {
    if (defaultDatasource && !datasourceId) {
      setDatasourceId(defaultDatasource.id);
    }
  }, [defaultDatasource, datasourceId]);

  const current = datasources.find((d) => d.id === datasourceId);
  const queryLanguage = current ? getQueryLanguage(current.plugin_type) : 'promql';

  if (!datasources.length) {
    return (
      <Card title='数据探索'>
        <Empty description='请先配置数据源'>
          <a href='/datasources'>前往数据源管理</a>
        </Empty>
      </Card>
    );
  }

  return (
    <Card
      title='数据探索'
      extra={
        <Select
          style={{ width: 260 }}
          value={datasourceId}
          options={datasources.map((d) => ({
            label: `${d.name} (${d.plugin_type})`,
            value: d.id,
          }))}
          onChange={setDatasourceId}
        />
      }
    >
      {queryLanguage === 'sql' ? (
        <>
          <Alert
            type='info'
            showIcon
            style={{ marginBottom: 16 }}
            message='ROS2 MCAP 数据源：MCAP 已导入 DuckDB，使用 SQL 查询 SystemStats 等指标。Ctrl+Enter 执行。'
          />
          {datasourceId && (
            <SqlGraph
              key={datasourceId}
              datasourceId={datasourceId}
              ingestStatus={current?.ingest_status}
              ingestInfo={current?.ingest_info}
            />
          )}
        </>
      ) : (
        <>
          <Alert
            type='info'
            showIcon
            style={{ marginBottom: 16 }}
            message='SQLite 数据源：通过 PromQL 查询时序指标。'
          />
          {datasourceId && <PromGraph key={datasourceId} datasourceId={datasourceId} />}
        </>
      )}
    </Card>
  );
}
