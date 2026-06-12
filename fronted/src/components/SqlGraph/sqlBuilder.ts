import type { SchemaTable } from '@/types';

const MAIN_TABLE = 'system_stats';
const SKIP_CHART_COLUMNS = new Set(['msg_id', 'idx', 'seq']);

const LABEL_COLUMNS = ['name', 'cpu_name', 'mount_point', 'node', 'topic'];

export function isNumericColumnType(type: string): boolean {
  const upper = type.toUpperCase();
  return ['INT', 'FLOAT', 'DOUBLE', 'DECIMAL', 'REAL', 'UINT', 'HUGE'].some((k) => upper.includes(k));
}

function getTableAlias(tableName: string): string {
  const part = tableName.replace(/^system_stats_?/, '') || 'main';
  return part.charAt(0) || 't';
}

function findLabelColumn(table: SchemaTable): string | undefined {
  return table.columns.find((c) => LABEL_COLUMNS.includes(c.name))?.name;
}

/** 根据 Schema 字段点击生成 SQL */
export function buildSqlFromField(
  tableName: string,
  columnName: string,
  columnType: string,
  schema: SchemaTable[],
): string {
  const table = schema.find((t) => t.name === tableName);
  if (!table) {
    return `SELECT ${columnName} FROM ${tableName} ORDER BY 1`;
  }

  if (tableName === MAIN_TABLE) {
    if (columnName === '_time') {
      return `SELECT _time FROM ${MAIN_TABLE} ORDER BY _time`;
    }
    if (SKIP_CHART_COLUMNS.has(columnName)) {
      return `SELECT _time, ${columnName} FROM ${MAIN_TABLE} ORDER BY _time`;
    }
    return `SELECT _time, ${columnName} FROM ${MAIN_TABLE} ORDER BY _time`;
  }

  const alias = getTableAlias(tableName);
  const labelCol = findLabelColumn(table);
  const isNumeric = isNumericColumnType(columnType);

  if (isNumeric && labelCol && columnName !== labelCol) {
    return `SELECT s._time, ${alias}.${labelCol}, ${alias}.${columnName}
FROM ${MAIN_TABLE} s
JOIN ${tableName} ${alias} ON ${alias}.msg_id = s.msg_id
ORDER BY s._time, ${alias}.${labelCol}`;
  }

  if (columnName === '_time') {
    return `SELECT s._time FROM ${MAIN_TABLE} s
JOIN ${tableName} ${alias} ON ${alias}.msg_id = s.msg_id
ORDER BY s._time`;
  }

  return `SELECT s._time, ${alias}.${columnName}
FROM ${MAIN_TABLE} s
JOIN ${tableName} ${alias} ON ${alias}.msg_id = s.msg_id
ORDER BY s._time`;
}

/** 点击表名：预览该表前 100 行 */
export function buildSqlFromTable(tableName: string, schema: SchemaTable[]): string {
  const table = schema.find((t) => t.name === tableName);
  if (!table) {
    return `SELECT * FROM ${tableName} LIMIT 100`;
  }

  if (tableName === MAIN_TABLE) {
    return `SELECT * FROM ${MAIN_TABLE} ORDER BY _time LIMIT 100`;
  }

  const alias = getTableAlias(tableName);
  return `SELECT s._time, ${alias}.*
FROM ${MAIN_TABLE} s
JOIN ${tableName} ${alias} ON ${alias}.msg_id = s.msg_id
ORDER BY s._time
LIMIT 100`;
}

export function parseSchemaTreeKey(key: string): { table: string; column?: string } | null {
  if (!key) return null;
  const dot = key.indexOf('.');
  if (dot === -1) return { table: key };
  return { table: key.slice(0, dot), column: key.slice(dot + 1) };
}
