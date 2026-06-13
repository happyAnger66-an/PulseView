import { useEffect, useState } from 'react';
import { Button, Card, Form, Input, Select, Space, Alert, Spin } from 'antd';
import { inspectSource } from '@/services/mcap';
import type { DatasourceFormProps, McapTopic } from '@/types';
import { DEFAULT_MSG_TYPE, PLUGIN_NAME, PLUGIN_TYPE } from './constants';

export default function ProtobufDatasourceForm({ action, data, onFinish, submitLoading }: DatasourceFormProps) {
  const [form] = Form.useForm();
  const [types, setTypes] = useState<McapTopic[]>([]);
  const [inspecting, setInspecting] = useState(false);
  const [inspectError, setInspectError] = useState('');

  useEffect(() => {
    if (data) form.setFieldsValue(data);
  }, [data, form]);

  const handleInspect = async () => {
    const path = form.getFieldValue(['settings', 'proto.path']);
    if (!path) {
      setInspectError('请先填写 Protobuf 文件路径');
      return;
    }
    setInspecting(true);
    setInspectError('');
    try {
      const res = await inspectSource(path, PLUGIN_TYPE);
      setTypes(res.topics);
      if (res.topics.length === 0) {
        setInspectError('未在该文件中发现消息');
      } else {
        form.setFieldValue(['settings', 'proto.msg_type'], res.topics[0].msg_type ?? DEFAULT_MSG_TYPE);
      }
    } catch (e) {
      setInspectError(e instanceof Error ? e.message : '扫描失败');
      setTypes([]);
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
          message='length-delimited protobuf 监控数据导入 DuckDB，可用 SQL 查询。'
        />
        <Form.Item name='name' label='名称' rules={[{ required: true, message: '请输入名称' }]}>
          <Input placeholder='例如 proto-metrics-001' />
        </Form.Item>
        <Form.Item label='Protobuf 文件路径' required>
          <Space.Compact style={{ width: '100%' }}>
            <Form.Item
              name={['settings', 'proto.path']}
              noStyle
              rules={[{ required: true, message: '请输入 Protobuf 路径' }]}
            >
              <Input placeholder='/path/to/metrics.pb' />
            </Form.Item>
            <Button loading={inspecting} onClick={handleInspect}>
              扫描
            </Button>
          </Space.Compact>
        </Form.Item>
        {inspectError && <Alert type='error' message={inspectError} style={{ marginBottom: 12 }} />}
        {inspecting && (
          <div style={{ marginBottom: 12 }}>
            <Spin size='small' /> 正在扫描...
          </div>
        )}
        <Form.Item
          name={['settings', 'proto.msg_type']}
          label='消息类型'
          rules={[{ required: true, message: '请选择消息类型' }]}
        >
          <Select
            placeholder='扫描后自动填充'
            options={(types.length ? types : [{ msg_type: DEFAULT_MSG_TYPE, name: DEFAULT_MSG_TYPE, message_count: 0 }]).map((t) => ({
              label: t.message_count ? `${t.msg_type} (${t.message_count} msgs)` : t.msg_type,
              value: t.msg_type ?? DEFAULT_MSG_TYPE,
            }))}
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
