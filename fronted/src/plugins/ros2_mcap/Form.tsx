import { useEffect, useState } from 'react';
import { Button, Card, Form, Input, Select, Space, Alert, Spin } from 'antd';
import { inspectMcap } from '@/services/mcap';
import type { DatasourceFormProps, McapTopic } from '@/types';
import { DEFAULT_MSG_TYPE, PLUGIN_NAME } from './constants';

export default function Ros2McapDatasourceForm({ action, data, onFinish, submitLoading }: DatasourceFormProps) {
  const [form] = Form.useForm();
  const [topics, setTopics] = useState<McapTopic[]>([]);
  const [inspecting, setInspecting] = useState(false);
  const [inspectError, setInspectError] = useState('');

  useEffect(() => {
    if (data) {
      form.setFieldsValue(data);
    }
  }, [data, form]);

  const handleInspect = async () => {
    const path = form.getFieldValue(['settings', 'mcap.path']);
    if (!path) {
      setInspectError('请先填写 MCAP 文件路径');
      return;
    }
    setInspecting(true);
    setInspectError('');
    try {
      const res = await inspectMcap(path);
      setTopics(res.topics);
      if (res.topics.length === 0) {
        setInspectError('未在该文件中发现 topic');
      }
    } catch (e) {
      setInspectError(e instanceof Error ? e.message : '扫描失败');
      setTopics([]);
    } finally {
      setInspecting(false);
    }
  };

  return (
    <Form form={form} layout='vertical' initialValues={data} onFinish={onFinish}>
      <Card title={`${action === 'add' ? '新增' : '编辑'} ${PLUGIN_NAME} 数据源`}>
        <Alert
          type='info'
          showIcon
          style={{ marginBottom: 16 }}
          message='MCAP 导入后写入 DuckDB，可通过 SQL 查询 SystemStats 等 ROS2 消息。'
        />
        <Form.Item name='name' label='名称' rules={[{ required: true, message: '请输入名称' }]}>
          <Input placeholder='例如 vehicle-log-001' />
        </Form.Item>
        <Form.Item label='MCAP 文件路径' required>
          <Space.Compact style={{ width: '100%' }}>
            <Form.Item
              name={['settings', 'mcap.path']}
              noStyle
              rules={[{ required: true, message: '请输入 MCAP 路径' }]}
            >
              <Input placeholder='/path/to/recording.mcap' />
            </Form.Item>
            <Button loading={inspecting} onClick={handleInspect}>
              扫描 Topic
            </Button>
          </Space.Compact>
        </Form.Item>
        {inspectError && <Alert type='error' message={inspectError} style={{ marginBottom: 12 }} />}
        {inspecting && (
          <div style={{ marginBottom: 12 }}>
            <Spin size='small' /> 正在扫描 MCAP...
          </div>
        )}
        <Form.Item
          name={['settings', 'mcap.topic']}
          label='Topic'
          rules={[{ required: true, message: '请选择 topic' }]}
        >
          <Select
            placeholder='先扫描 MCAP 后选择 topic'
            options={topics.map((t) => ({
              label: `${t.name} (${t.message_count} msgs)`,
              value: t.name,
            }))}
            onChange={(topicName) => {
              const topic = topics.find((t) => t.name === topicName);
              form.setFieldValue(['settings', 'mcap.msg_type'], topic?.msg_type || DEFAULT_MSG_TYPE);
            }}
          />
        </Form.Item>
        <Form.Item
          name={['settings', 'mcap.msg_type']}
          label='Msg 类型'
          rules={[{ required: true, message: '请选择 msg 类型' }]}
        >
          <Select
            options={[
              { label: DEFAULT_MSG_TYPE, value: DEFAULT_MSG_TYPE },
            ]}
          />
        </Form.Item>
        <Form.Item name='description' label='描述'>
          <Input.TextArea rows={2} />
        </Form.Item>
        <Form.Item>
          <Button type='primary' htmlType='submit' loading={submitLoading}>
            保存并导入 DuckDB
          </Button>
        </Form.Item>
      </Card>
    </Form>
  );
}
