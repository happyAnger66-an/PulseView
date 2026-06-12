import { useEffect, useState } from 'react';
import { Card, Table, Button, Popconfirm, message, Tag, Dropdown, Menu, Tooltip } from 'antd';
import { DownOutlined, PlusOutlined } from '@ant-design/icons';
import { useHistory } from 'react-router-dom';
import { deleteDatasource, getDatasources } from '@/services/datasource';
import { getPluginList } from '@/plugins';
import type { Datasource } from '@/types';
import { useAppContext } from '@/components/Layout';

function renderPath(record: Datasource) {
  if (record.plugin_type === 'ros2_mcap') {
    const s = record.settings as { 'mcap.path'?: string; 'mcap.topic'?: string };
    return `${s['mcap.path'] ?? '-'} → ${s['mcap.topic'] ?? '-'}`;
  }
  const s = record.settings as { 'sqlite.path'?: string };
  return s['sqlite.path'] ?? '-';
}

export default function DatasourcesPage() {
  const history = useHistory();
  const { reloadDatasources } = useAppContext();
  const [list, setList] = useState<Datasource[]>([]);
  const [loading, setLoading] = useState(false);
  const plugins = getPluginList();

  const load = async () => {
    setLoading(true);
    try {
      const data = await getDatasources();
      setList(data);
    } finally {
      setLoading(false);
    }
  };

  useEffect(() => {
    load();
  }, []);

  const handleDelete = async (id: number) => {
    await deleteDatasource(id);
    message.success('已删除');
    await load();
    await reloadDatasources();
  };

  const addMenu = (
    <Menu
      onClick={({ key }) => history.push(`/datasources/add/${key}`)}
      items={plugins.map((p) => ({ key: p.type, label: p.name }))}
    />
  );

  return (
    <Card
      title='数据源管理'
      extra={
        <Dropdown overlay={addMenu}>
          <Button type='primary' icon={<PlusOutlined />}>
            添加数据源 <DownOutlined />
          </Button>
        </Dropdown>
      }
    >
      <div style={{ marginBottom: 16, color: '#8c8c8c' }}>
        支持插件：{plugins.map((p) => `${p.name}(${p.queryLanguage})`).join('、')}
      </div>
      <Table
        rowKey='id'
        loading={loading}
        dataSource={list}
        columns={[
          { title: '名称', dataIndex: 'name' },
          {
            title: '类型',
            dataIndex: 'plugin_type',
            render: (v: string) => <Tag color={v === 'ros2_mcap' ? 'blue' : 'purple'}>{v}</Tag>,
          },
          {
            title: '路径 / Topic',
            render: (_, r) => renderPath(r),
          },
          {
            title: '状态',
            render: (_, r) => {
              if (r.plugin_type !== 'ros2_mcap') return '-';
              const errMsg = r.ingest_info?.message as string | undefined;
              const tag = (
                <Tag color={r.ingest_status === 'ready' ? 'green' : r.ingest_status === 'error' ? 'red' : 'default'}>
                  {r.ingest_status || 'pending'}
                </Tag>
              );
              return errMsg ? <Tooltip title={errMsg}>{tag}</Tooltip> : tag;
            },
          },
          {
            title: '操作',
            render: (_, record) => (
              <>
                <Button
                  type='link'
                  onClick={() => history.push(`/datasources/edit/${record.plugin_type}/${record.id}`)}
                >
                  编辑
                </Button>
                <Popconfirm title='确认删除？' onConfirm={() => handleDelete(record.id)}>
                  <Button type='link' danger>
                    删除
                  </Button>
                </Popconfirm>
              </>
            ),
          },
        ]}
      />
    </Card>
  );
}
