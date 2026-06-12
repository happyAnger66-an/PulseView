import { Table } from 'antd';
import type { PromMatrixResult } from '@/types';

interface Props {
  data: PromMatrixResult[];
}

export default function ResultTable({ data }: Props) {
  const rows = data.flatMap((item, idx) => {
    const labels = Object.entries(item.metric)
      .map(([k, v]) => `${k}="${v}"`)
      .join(', ');
    const last = item.values[item.values.length - 1];
    return {
      key: idx,
      metric: labels || item.metric.__name__ || '-',
      lastValue: last?.[1] ?? '-',
      points: item.values.length,
    };
  });

  return (
    <Table
      size='small'
      pagination={false}
      dataSource={rows}
      columns={[
        { title: '序列', dataIndex: 'metric', key: 'metric' },
        { title: '最新值', dataIndex: 'lastValue', key: 'lastValue', width: 120 },
        { title: '点数', dataIndex: 'points', key: 'points', width: 80 },
      ]}
    />
  );
}
