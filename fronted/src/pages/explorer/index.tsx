import { useEffect, useState } from 'react';
import { Card, Select, Empty, Alert } from 'antd';
import { useAppContext } from '@/components/Layout';
import { getPlugin } from '@/plugins';
import '@/visualizations';

export default function ExplorerPage() {
  const { datasources, defaultDatasource } = useAppContext();
  const [datasourceId, setDatasourceId] = useState<number>();

  useEffect(() => {
    if (defaultDatasource && !datasourceId) {
      setDatasourceId(defaultDatasource.id);
    }
  }, [defaultDatasource, datasourceId]);

  const current = datasources.find((d) => d.id === datasourceId);
  const plugin = current ? getPlugin(current.plugin_type) : undefined;

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
      {current && plugin ? (
        <plugin.QueryPanel datasource={current} />
      ) : current ? (
        <Alert type='warning' showIcon message={`未注册的数据源类型：${current.plugin_type}`} />
      ) : null}
    </Card>
  );
}
