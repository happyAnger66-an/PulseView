import { useCallback, useEffect, useMemo, useState } from 'react';
import { Alert, Button, Input, Tabs, Tree, Tag, Space, message } from 'antd';
import type { DataNode } from 'antd/es/tree';
import { getDatasourceSchema, runSqlQuery, ingestDatasource } from '@/services/sql';
import type { SchemaTable, SqlQueryResult } from '@/types';
import { selectViz } from '@/visualizations';
import {
  buildSqlFromField,
  buildSqlFromTable,
  buildDefaultMetricSql,
  getTableShortName,
  parseSchemaTreeKey,
} from './sqlBuilder';
import './style.less';

const { TabPane } = Tabs;

/** 由 schema 元数据生成预设查询：每张声明了 default_metrics 的表一个按钮 */
function buildPresets(schema: SchemaTable[]): { label: string; sql: string }[] {
  return schema
    .map((table) => ({ table, sql: buildDefaultMetricSql(table) }))
    .filter((p): p is { table: SchemaTable; sql: string } => Boolean(p.sql))
    .map(({ table, sql }) => ({ label: getTableShortName(table), sql }));
}

interface Props {
  datasourceId: number;
  ingestStatus?: string;
  ingestInfo?: Record<string, unknown>;
}

export default function SqlGraph({ datasourceId, ingestStatus, ingestInfo }: Props) {
  const [sql, setSql] = useState<string>('');
  const [loading, setLoading] = useState(false);
  const [reingesting, setReingesting] = useState(false);
  const [error, setError] = useState('');
  const [result, setResult] = useState<SqlQueryResult | null>(null);
  const [schema, setSchema] = useState<SchemaTable[]>([]);
  const [tab, setTab] = useState<string>('timeseries');
  const [selectedKey, setSelectedKey] = useState<string>();

  const loadSchema = async () => {
    try {
      const res = await getDatasourceSchema(datasourceId);
      setSchema(res.tables);
    } catch {
      setSchema([]);
    }
  };

  useEffect(() => {
    loadSchema();
  }, [datasourceId]);

  const presets = useMemo(() => buildPresets(schema), [schema]);

  // schema 加载后，把主表默认指标查询填入编辑器作为初始 SQL
  useEffect(() => {
    if (sql) return;
    const mainTable = schema.find((t) => !t.parent_table && (t.default_metrics ?? []).length);
    const defaultSql = mainTable ? buildDefaultMetricSql(mainTable) : presets[0]?.sql;
    if (defaultSql) setSql(defaultSql);
  }, [schema, presets, sql]);

  const executeQuery = useCallback(
    async (querySql?: string) => {
      const q = (querySql ?? sql).trim();
      if (!q) return;
      setSql(q);
      setLoading(true);
      setError('');
      try {
        const res = await runSqlQuery(datasourceId, q);
        setResult(res);
        // 选出适用的可视化，默认优先非表格（如时序图）
        const vizzes = selectViz(res.meta);
        const preferred = vizzes.find((v) => v.type !== 'table') ?? vizzes[0];
        if (preferred) setTab(preferred.type);
      } catch (e) {
        setError(e instanceof Error ? e.message : '查询失败');
        setResult(null);
      } finally {
        setLoading(false);
      }
    },
    [datasourceId, sql],
  );

  const handleSchemaSelect = useCallback(
    (keys: React.Key[]) => {
      const key = String(keys[0] ?? '');
      if (!key) return;
      setSelectedKey(key);

      const parsed = parseSchemaTreeKey(key);
      if (!parsed) return;

      let generated = '';
      if (parsed.column) {
        const table = schema.find((t) => t.name === parsed.table);
        const col = table?.columns.find((c) => c.name === parsed.column);
        if (!col) return;
        generated = buildSqlFromField(parsed.table, parsed.column, col.type, schema);
      } else {
        generated = buildSqlFromTable(parsed.table, schema);
      }

      setSql(generated);
      executeQuery(generated);
    },
    [schema, executeQuery],
  );

  const handleReingest = async () => {
    setReingesting(true);
    try {
      await ingestDatasource(datasourceId, true);
      message.success('重新导入完成');
      await loadSchema();
    } catch (e) {
      message.error(e instanceof Error ? e.message : '导入失败');
    } finally {
      setReingesting(false);
    }
  };

  const treeData: DataNode[] = schema.map((table) => ({
    title: table.name,
    key: table.name,
    selectable: true,
    children: table.columns.map((col) => ({
      title: `${col.name} (${col.type})`,
      key: `${table.name}.${col.name}`,
      isLeaf: true,
      selectable: true,
    })),
  }));

  const ingestError =
    ingestStatus === 'error' && ingestInfo?.message ? String(ingestInfo.message) : '';

  return (
    <div className='sql-graph'>
      <div className='sql-graph-header'>
        <Space direction='vertical' size={4} style={{ width: '100%' }}>
          <Space>
            <Tag color={ingestStatus === 'ready' ? 'green' : ingestStatus === 'error' ? 'red' : 'default'}>
              ingest: {ingestStatus || 'unknown'}
            </Tag>
            <Button size='small' loading={reingesting} onClick={handleReingest}>
              重新导入
            </Button>
          </Space>
          {ingestError ? <Alert type='error' showIcon message={ingestError} /> : null}
        </Space>
        <Space wrap>
          {presets.map((q) => (
            <Button
              key={q.label}
              size='small'
              onClick={() => {
                setSql(q.sql);
                executeQuery(q.sql);
              }}
            >
              {q.label}
            </Button>
          ))}
        </Space>
      </div>

      <div className='sql-graph-body'>
        <div className='sql-graph-schema'>
          <div className='schema-title'>DuckDB Schema</div>
          <div className='schema-hint'>点击字段自动生成 SQL 并查询</div>
          {treeData.length ? (
            <Tree
              className='schema-tree'
              treeData={treeData}
              defaultExpandAll
              selectedKeys={selectedKey ? [selectedKey] : []}
              onSelect={handleSchemaSelect}
            />
          ) : (
            <Alert type='warning' message='暂无表结构，请先完成 MCAP 导入' />
          )}
        </div>
        <div className='sql-graph-main'>
          <div className='sql-input-row'>
            <Input.TextArea
              className='sql-input'
              value={sql}
              autoSize={{ minRows: 4, maxRows: 10 }}
              onChange={(e) => setSql(e.target.value)}
              onKeyDown={(e) => {
                if (e.key === 'Enter' && (e.ctrlKey || e.metaKey)) {
                  e.preventDefault();
                  executeQuery();
                }
              }}
            />
            <Button type='primary' loading={loading} onClick={() => executeQuery()}>
              查询
            </Button>
          </div>
          {error && <Alert type='error' message={error} showIcon style={{ marginBottom: 12 }} />}
          {result && (() => {
            const vizzes = selectViz(result.meta);
            const active = vizzes.find((v) => v.type === tab) ?? vizzes[0];
            const ActiveCpt = active?.component;
            return (
              <>
                <Tabs activeKey={active?.type} onChange={setTab}>
                  {vizzes.map((v) => (
                    <TabPane tab={`${v.name} (${result.meta.row_count})`} key={v.type} />
                  ))}
                </Tabs>
                {ActiveCpt && (
                  <ActiveCpt columns={result.columns} rows={result.rows} meta={result.meta} />
                )}
              </>
            );
          })()}
        </div>
      </div>
    </div>
  );
}
