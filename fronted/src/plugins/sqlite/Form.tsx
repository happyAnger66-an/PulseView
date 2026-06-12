import { Form, Input, Card, Button } from 'antd';
import type { DatasourceFormProps } from '@/types';
import { PLUGIN_NAME } from './constants';

export default function SqliteDatasourceForm({ action, data, onFinish, submitLoading }: DatasourceFormProps) {
  const [form] = Form.useForm();

  return (
    <Form
      form={form}
      layout='vertical'
      initialValues={data}
      onFinish={onFinish}
    >
      <Card title={`${action === 'add' ? '新增' : '编辑'} ${PLUGIN_NAME} 数据源`}>
        <Form.Item name='name' label='名称' rules={[{ required: true, message: '请输入名称' }]}>
          <Input placeholder='例如 local-metrics' />
        </Form.Item>
        <Form.Item name={['settings', 'sqlite.path']} label='数据库路径' rules={[{ required: true, message: '请输入 SQLite 文件路径' }]}>
          <Input placeholder='例如 ./data/metrics.db' />
        </Form.Item>
        <Form.Item name='description' label='描述'>
          <Input.TextArea rows={2} placeholder='可选描述' />
        </Form.Item>
        <Form.Item>
          <Button type='primary' htmlType='submit' loading={submitLoading}>
            保存
          </Button>
        </Form.Item>
      </Card>
    </Form>
  );
}
