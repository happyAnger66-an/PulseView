import { Table } from 'antd';
import type { VizProps } from './types';

export default function TableViz({ columns, rows }: VizProps) {
  const dataSource = rows.map((row, idx) => {
    const record: Record<string, unknown> = { key: idx };
    columns.forEach((col, i) => {
      record[col] = row[i];
    });
    return record;
  });

  return (
    <Table
      size='small'
      scroll={{ x: true }}
      pagination={{ pageSize: 50 }}
      dataSource={dataSource}
      columns={columns.map((col) => ({
        title: col,
        dataIndex: col,
        key: col,
        ellipsis: true,
      }))}
    />
  );
}
