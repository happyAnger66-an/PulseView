import { useEffect, useState } from 'react';
import { Alert, Button, Card, Form, Input, Space, Spin } from 'antd';
import { inspectSource } from '@/services/mcap';
import type { DatasourceFormProps, McapTopic } from '@/types';
import { PLUGIN_NAME, PLUGIN_TYPE } from './constants';

export default function PerfettoDatasourceForm({ action, data, onFinish, submitLoading }: DatasourceFormProps) {
  const [form] = Form.useForm();
  const [tracks, setTracks] = useState<McapTopic[]>([]);
  const [inspecting, setInspecting] = useState(false);
  const [inspectError, setInspectError] = useState('');

  useEffect(() => {
    if (data) form.setFieldsValue(data);
  }, [data, form]);

  const handleInspect = async () => {
    const path = form.getFieldValue(['settings', 'perfetto.path']);
    if (!path) {
      setInspectError('请先填写 Perfetto trace 文件路径');
      return;
    }
    setInspecting(true);
    setInspectError('');
    try {
      const res = await inspectSource(path, PLUGIN_TYPE);
      setTracks(res.topics);
      if (res.topics.length === 0) setInspectError('未在该 trace 中发现区间');
    } catch (e) {
      setInspectError(e instanceof Error ? e.message : '扫描失败');
      setTracks([]);
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
          message='指向 Perfetto trace 文件（.perfetto-trace / .pftrace / Chrome JSON 等）。slice 区间将转为 span 表，支持 Timeline 泳道视图。需后端可用 trace_processor_shell。'
        />
        <Form.Item name='name' label='名称' rules={[{ required: true, message: '请输入名称' }]}>
          <Input placeholder='例如 perfetto-trace-001' />
        </Form.Item>
        <Form.Item label='Perfetto trace 文件' required>
          <Space.Compact style={{ width: '100%' }}>
            <Form.Item
              name={['settings', 'perfetto.path']}
              noStyle
              rules={[{ required: true, message: '请输入 Perfetto trace 文件路径' }]}
            >
              <Input placeholder='/path/to/trace.perfetto-trace' />
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
        {tracks.length > 0 && (
          <Alert
            type='success'
            style={{ marginBottom: 12 }}
            message={`发现 ${tracks.length} 个泳道：${tracks.map((t) => `${t.name}(${t.message_count})`).join('、')}`}
          />
        )}
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
