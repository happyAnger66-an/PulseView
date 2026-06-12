import type { SchemaTable } from '@/types';

const MAIN_TABLE = 'system_stats';

function getTableAlias(tableName: string): string {
  const part = tableName.replace(/^system_stats_?/, '') || 'main';
  return part.charAt(0) || 't';
}

function orderByClause(alias: string, dims: string[], timeCol = 's._time'): string {
  const parts = [timeCol, ...dims.map((d) => `${alias}.${d}`)];
  return `ORDER BY ${parts.join(', ')}`;
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
    return `SELECT _time, ${columnName} FROM ${MAIN_TABLE} ORDER BY _time`;
  }

  const alias = getTableAlias(tableName);
  const dims = table.dimension_keys ?? [];
  const dimSelect = dims.map((d) => `${alias}.${d}`).join(', ');
  const isNumeric = isNumericColumnType(columnType);

  if (isNumeric && dims.length && !dims.includes(columnName)) {
    const selectDims = dimSelect ? `, ${dimSelect}` : '';
    return `SELECT s._time${selectDims}, ${alias}.${columnName}
FROM ${MAIN_TABLE} s
JOIN ${tableName} ${alias} ON ${alias}.msg_id = s.msg_id
${orderByClause(alias, dims)}`;
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
  const dims = table.dimension_keys ?? [];
  const order = dims.length ? orderByClause(alias, dims) : 'ORDER BY s._time';

  return `SELECT s._time, ${alias}.*
FROM ${MAIN_TABLE} s
JOIN ${tableName} ${alias} ON ${alias}.msg_id = s.msg_id
${order}
LIMIT 100`;
}

export function parseSchemaTreeKey(key: string): { table: string; column?: string } | null {
  if (!key) return null;
  const dot = key.indexOf('.');
  if (dot === -1) return { table: key };
  return { table: key.slice(0, dot), column: key.slice(dot + 1) };
}

export function isNumericColumnType(type: string): boolean {
  const upper = type.toUpperCase();
  return ['INT', 'FLOAT', 'DOUBLE', 'DECIMAL', 'REAL', 'UINT', 'HUGE'].some((k) => upper.includes(k));
}

/** 根据表元数据生成默认指标查询 */
export function buildDefaultMetricSql(table: SchemaTable, metric?: string): string | null {
  const metrics = table.default_metrics ?? [];
  const target = metric ?? metrics[0];
  if (!target) return null;

  if (!table.parent_table) {
    return `SELECT _time, ${target} FROM ${table.name} ORDER BY _time`;
  }

  const alias = getTableAlias(table.name);
  const dims = table.dimension_keys ?? [];
  const dimSelect = dims.length ? `, ${dims.map((d) => `${alias}.${d}`).join(', ')}` : '';

  return `SELECT s._time${dimSelect}, ${alias}.${target}
FROM ${MAIN_TABLE} s
JOIN ${table.name} ${alias} ON ${alias}.msg_id = s.msg_id
${orderByClause(alias, dims)}`;
}
