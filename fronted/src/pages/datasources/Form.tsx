import { useEffect, useState } from 'react';
import { message } from 'antd';
import { useHistory, useParams, useLocation } from 'react-router-dom';
import { createDatasource, getDatasource, updateDatasource } from '@/services/datasource';
import { getPlugin } from '@/plugins';
import { useAppContext } from '@/components/Layout';

export default function DatasourceFormPage() {
  const history = useHistory();
  const location = useLocation();
  const { type, id } = useParams<{ type: string; id?: string }>();
  const action = location.pathname.includes('/edit/') ? 'edit' : 'add';
  const { reloadDatasources } = useAppContext();
  const [loading, setLoading] = useState(false);
  const [data, setData] = useState<any>();
  const plugin = getPlugin(type || 'sqlite');

  useEffect(() => {
    if (action === 'edit' && id) {
      getDatasource(Number(id)).then(setData);
    }
  }, [action, id]);

  if (!plugin) return null;
  const FormCpt = plugin.DatasourceForm;

  const onFinish = async (values: any) => {
    setLoading(true);
    try {
      const pluginType = plugin?.type || type || 'sqlite';
      if (action === 'add') {
        await createDatasource({ ...values, plugin_type: pluginType });
        message.success('创建成功');
      } else if (id) {
        await updateDatasource(Number(id), values);
        message.success('更新成功');
      }
      await reloadDatasources();
      history.push('/datasources');
    } catch (e) {
      message.error(e instanceof Error ? e.message : '保存失败');
    } finally {
      setLoading(false);
    }
  };

  return (
    <FormCpt
      action={action}
      data={data}
      submitLoading={loading}
      onFinish={onFinish}
    />
  );
}
