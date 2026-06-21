import type { SchemaTable } from '@/types';

const TIME_COLUMN = '_time';
const DEFAULT_JOIN_KEY = 'msg_id';
// 子表 JOIN 主表时主表固定用别名 s
const PARENT_ALIAS = 's';

/** 去掉主表前缀后的短名，用于展示与别名生成 */
export function getTableShortName(table: SchemaTable): string {
  if (!table.parent_table) return table.name;
  const prefix = `${table.parent_table}_`;
  return table.name.startsWith(prefix) ? table.name.slice(prefix.length) : table.name;
}

function getTableAlias(table: SchemaTable): string {
  const ch = getTableShortName(table).charAt(0) || 't';
  return ch === PARENT_ALIAS ? 't' : ch;
}

function joinClause(table: SchemaTable, alias: string): string {
  const joinKey = table.join_key ?? DEFAULT_JOIN_KEY;
  return `FROM ${table.parent_table} ${PARENT_ALIAS}
JOIN ${table.name} ${alias} ON ${alias}.${joinKey} = ${PARENT_ALIAS}.${joinKey}`;
}

function orderByClause(alias: string, dims: string[], timeCol = `${PARENT_ALIAS}.${TIME_COLUMN}`): string {
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

  if (!table.parent_table) {
    if (columnName === TIME_COLUMN) {
      return `SELECT ${TIME_COLUMN} FROM ${tableName} ORDER BY ${TIME_COLUMN}`;
    }
    return `SELECT ${TIME_COLUMN}, ${columnName} FROM ${tableName} ORDER BY ${TIME_COLUMN}`;
  }

  const alias = getTableAlias(table);
  const dims = table.dimension_keys ?? [];
  const dimSelect = dims.map((d) => `${alias}.${d}`).join(', ');
  const isNumeric = isNumericColumnType(columnType);

  if (isNumeric && dims.length && !dims.includes(columnName)) {
    const selectDims = dimSelect ? `, ${dimSelect}` : '';
    return `SELECT ${PARENT_ALIAS}.${TIME_COLUMN}${selectDims}, ${alias}.${columnName}
${joinClause(table, alias)}
${orderByClause(alias, dims)}`;
  }

  if (columnName === TIME_COLUMN) {
    return `SELECT ${PARENT_ALIAS}.${TIME_COLUMN} ${joinClause(table, alias)}
ORDER BY ${PARENT_ALIAS}.${TIME_COLUMN}`;
  }

  return `SELECT ${PARENT_ALIAS}.${TIME_COLUMN}, ${alias}.${columnName}
${joinClause(table, alias)}
ORDER BY ${PARENT_ALIAS}.${TIME_COLUMN}`;
}

/** 点击表名：预览该表前 100 行 */
export function buildSqlFromTable(tableName: string, schema: SchemaTable[]): string {
  const table = schema.find((t) => t.name === tableName);
  if (!table) {
    return `SELECT * FROM ${tableName} LIMIT 100`;
  }

  if (!table.parent_table) {
    return `SELECT * FROM ${tableName} ORDER BY ${TIME_COLUMN} LIMIT 100`;
  }

  const alias = getTableAlias(table);
  const dims = table.dimension_keys ?? [];
  const order = dims.length ? orderByClause(alias, dims) : `ORDER BY ${PARENT_ALIAS}.${TIME_COLUMN}`;

  return `SELECT ${PARENT_ALIAS}.${TIME_COLUMN}, ${alias}.*
${joinClause(table, alias)}
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

/** span 类表：拉全列，Timeline 才能分泳道并在详情里展示 meta */
function buildSpanSql(table: SchemaTable): string {
  const names = table.columns.map((c) => c.name);
  const priority = [
    TIME_COLUMN,
    '_dur',
    ...(table.dimension_keys ?? []),
    'name',
    'category',
    'span_id',
    'cpu_id',
    'depth',
  ];
  const ordered: string[] = [];
  for (const col of priority) {
    if (names.includes(col) && !ordered.includes(col)) ordered.push(col);
  }
  for (const col of names) {
    if (!ordered.includes(col)) ordered.push(col);
  }
  return `SELECT ${ordered.join(', ')} FROM ${table.name} ORDER BY ${TIME_COLUMN}`;
}

/** 根据表元数据生成默认指标查询；不传 metrics 时使用全部 default_metrics */
export function buildDefaultMetricSql(table: SchemaTable, metrics?: string | string[]): string | null {
  if (table.table_kind === 'span') {
    return buildSpanSql(table);
  }
  const defaults = table.default_metrics ?? [];
  const targets = metrics === undefined ? defaults : Array.isArray(metrics) ? metrics : [metrics];
  if (!targets.length) return null;

  if (!table.parent_table) {
    return `SELECT ${TIME_COLUMN}, ${targets.join(', ')} FROM ${table.name} ORDER BY ${TIME_COLUMN}`;
  }

  const alias = getTableAlias(table);
  const dims = table.dimension_keys ?? [];
  const dimSelect = dims.length ? `, ${dims.map((d) => `${alias}.${d}`).join(', ')}` : '';
  const metricSelect = targets.map((m) => `${alias}.${m}`).join(', ');

  return `SELECT ${PARENT_ALIAS}.${TIME_COLUMN}${dimSelect}, ${metricSelect}
${joinClause(table, alias)}
${orderByClause(alias, dims)}`;
}
