import { Table } from 'antd';
import type { ColumnsType } from 'antd/es/table';
import type { SeriesStats } from '@/utils/stats';
import { formatStatValue } from '@/utils/stats';

interface Props {
  rows: SeriesStats[];
}

export default function SeriesStatsTable({ rows }: Props) {
  const columns: ColumnsType<SeriesStats> = [
    {
      title: '曲线',
      dataIndex: 'label',
      key: 'label',
      ellipsis: true,
    },
    {
      title: '样本数',
      dataIndex: 'count',
      key: 'count',
      width: 72,
      align: 'right',
    },
    {
      title: 'Avg',
      dataIndex: 'avg',
      key: 'avg',
      width: 88,
      align: 'right',
      render: (v: number | null) => formatStatValue(v),
    },
    {
      title: 'P50',
      dataIndex: 'p50',
      key: 'p50',
      width: 88,
      align: 'right',
      render: (v: number | null) => formatStatValue(v),
    },
    {
      title: 'P99',
      dataIndex: 'p99',
      key: 'p99',
      width: 88,
      align: 'right',
      render: (v: number | null) => formatStatValue(v),
    },
  ];

  if (!rows.length) return null;

  return (
    <Table
      className='timeseries-stats-table'
      size='small'
      pagination={false}
      rowKey='label'
      columns={columns}
      dataSource={rows}
      scroll={{ x: true, y: 200 }}
    />
  );
}
