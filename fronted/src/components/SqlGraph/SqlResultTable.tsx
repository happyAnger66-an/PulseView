import { Table } from 'antd';
import type { SqlQueryResult } from '@/types';

interface Props {
  result: SqlQueryResult;
}

export default function SqlResultTable({ result }: Props) {
  const dataSource = result.rows.map((row, idx) => {
    const record: Record<string, unknown> = { key: idx };
    result.columns.forEach((col, i) => {
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
      columns={result.columns.map((col) => ({
        title: col,
        dataIndex: col,
        key: col,
        ellipsis: true,
      }))}
    />
  );
}
