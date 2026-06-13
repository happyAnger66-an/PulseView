# PulseView 架构图

本文档汇总当前（阶段 0–4 完成后）前后端的**类图**、**架构图**与**端到端流程图**。
所有图使用 Mermaid 绘制，可在支持 Mermaid 的 Markdown 预览中查看。

- 后端：FastAPI + DuckDB，两级可扩展（格式级 `FormatImporter` + 消息级 `RosMsgAdapter`）
- 前端：React + Antd，插件注册表（`PluginDefinition`）+ 图表注册表（`VizDefinition`）
- 中间契约：DuckDB 表 + `_pv_table_meta` 元数据 + 统一查询结果 `meta`

---

## 一、后端类图

```mermaid
classDiagram
    %% ====== 格式级：FormatImporter ======
    class FormatImporter {
        <<Protocol>>
        +str format_type
        +required_settings() list~str~
        +inspect(path) dict
        +ingest(db_path, settings) dict
    }
    class FormatImporterRegistry {
        -dict _importers
        +register(importer)
        +get(format_type) FormatImporter
        +has(format_type) bool
        +list_formats() list
    }
    class McapImporter {
        +format_type = "ros2_mcap"
        +ingest() 使用 ingest_mcap + RosMsgAdapter
    }
    class ProtobufImporter {
        +format_type = "protobuf"
        +ingest() 使用 proto_schema
    }
    class CtfImporter {
        +format_type = "ctf"
        +ingest() 使用 ctf_format, 配对 span
    }
    FormatImporter <|.. McapImporter
    FormatImporter <|.. ProtobufImporter
    FormatImporter <|.. CtfImporter
    FormatImporterRegistry o--> "*" FormatImporter

    %% ====== 消息级：RosMsgAdapter（MCAP 内部二级注册表）======
    class RosMsgAdapter {
        <<Protocol>>
        +str msg_type
        +tables() list~TableDef~
        +flatten(msg, ctx) dict
    }
    class RosMsgAdapterRegistry {
        -dict _adapters
        +register(adapter)
        +get(msg_type) RosMsgAdapter
        +list_msg_types() list
    }
    class SystemStatsAdapter {
        +msg_type = ".../SystemStats"
    }
    RosMsgAdapter <|.. SystemStatsAdapter
    RosMsgAdapterRegistry o--> "*" RosMsgAdapter
    McapImporter ..> RosMsgAdapterRegistry : 解码用

    %% ====== 表结构与元数据 ======
    class TableDef {
        +str name
        +list~ColumnDef~ columns
        +str parent_table
        +str join_key
        +list dimension_keys
        +list default_metrics
        +str table_kind
    }
    class ColumnDef {
        +str name
        +str duckdb_type
    }
    class IngestContext {
        +int msg_id
        +int time_us
    }
    TableDef o--> "*" ColumnDef
    RosMsgAdapter ..> TableDef
    ProtobufImporter ..> TableDef
    CtfImporter ..> TableDef

    %% ====== 持久化 / 查询引擎 ======
    class table_meta {
        <<module>>
        +META_TABLE = "_pv_table_meta"
        +write_table_meta(conn, tables)
        +read_table_meta(conn) dict
    }
    class duckdb_engine {
        <<module>>
        +get_schema(db_path) dict
        +run_query(db_path, sql) dict
        +_infer_columns() time/dur/dims/values
    }
    class DatasourceStore {
        +list/create/update/delete
        +PLUGIN_META
        +plugin_capabilities(type)
    }
    table_meta ..> TableDef
    duckdb_engine ..> table_meta : 读元数据
    ProtobufImporter ..> table_meta : 写元数据
    CtfImporter ..> table_meta
    McapImporter ..> table_meta

    %% ====== API 层 ======
    class FastAPI_main {
        <<module>>
        +/api/datasources CRUD
        +/api/inspect
        +/api/datasources/{id}/ingest
        +/api/datasources/{id}/schema
        +/api/sql/query
        +_try_ingest(ds_id)
    }
    FastAPI_main ..> FormatImporterRegistry : 分派 ingest/inspect
    FastAPI_main ..> duckdb_engine : schema/query
    FastAPI_main ..> DatasourceStore : 元信息
```

---

## 二、后端分层架构图

```mermaid
flowchart TB
    subgraph API["API 层 (main.py / FastAPI)"]
        EP_DS["数据源 CRUD"]
        EP_INSPECT["/api/inspect"]
        EP_INGEST["/api/.../ingest"]
        EP_SCHEMA["/api/.../schema"]
        EP_SQL["/api/sql/query"]
    end

    subgraph STORE["元信息层 (store.py)"]
        DS["DatasourceStore (内存)"]
        PM["PLUGIN_META + capabilities"]
    end

    subgraph INGEST["导入层 (ingest/)"]
        FR["FormatImporterRegistry"]
        MCAP["McapImporter"]
        PROTO["ProtobufImporter"]
        CTF["CtfImporter"]
        AR["RosMsgAdapterRegistry"]
        SS["SystemStatsAdapter"]
        TM["table_meta (_pv_table_meta)"]
    end

    subgraph ENGINE["查询层 (duckdb_engine.py)"]
        SCH["get_schema"]
        RQ["run_query + 列语义推断"]
    end

    subgraph STORAGE["存储 (DuckDB 文件/数据源)"]
        DB[("data/duckdb/*.duckdb")]
    end

    EP_DS --> DS
    EP_INSPECT --> FR
    EP_INGEST --> FR
    FR --> MCAP & PROTO & CTF
    MCAP --> AR --> SS
    MCAP & PROTO & CTF --> TM
    MCAP & PROTO & CTF --> DB
    TM --> DB
    EP_SCHEMA --> SCH --> DB
    EP_SQL --> RQ --> DB
    EP_INGEST -. capability 判定 .-> PM
    EP_SQL -. capability 判定 .-> PM
```

---

## 三、前端类/组件图

```mermaid
classDiagram
    %% ====== 数据源插件注册表 ======
    class PluginDefinition {
        +str type
        +str name
        +QueryLanguage queryLanguage
        +string[] capabilities
        +string[] defaultVisualizations
        +ComponentType DatasourceForm
        +ComponentType QueryPanel
    }
    class PluginRegistry {
        <<module>>
        +PLUGINS map type to PluginDefinition
        +getPlugin(type)
    }
    class Ros2McapPlugin
    class ProtobufPlugin
    class CtfPlugin
    class SqlitePlugin
    PluginRegistry o--> "*" PluginDefinition
    PluginDefinition <|.. Ros2McapPlugin
    PluginDefinition <|.. ProtobufPlugin
    PluginDefinition <|.. CtfPlugin
    PluginDefinition <|.. SqlitePlugin

    %% ====== 可视化注册表 ======
    class VizDefinition {
        +str type
        +str name
        +ComponentType component
        +accepts(meta) bool
    }
    class VizRegistry {
        <<module visualizations/>>
        +registerViz(def)
        +selectViz(meta) VizDefinition[]
    }
    class TimeseriesViz
    class TimelineViz
    class TableViz
    VizRegistry o--> "*" VizDefinition
    VizDefinition <|.. TimeseriesViz
    VizDefinition <|.. TimelineViz
    VizDefinition <|.. TableViz

    %% ====== 查询面板与 SQL 构建 ======
    class QueryPanel {
        <<per-plugin>>
        +datasource
    }
    class SqlGraph {
        +datasourceId
        +schema/presets/SQL 编辑器
        +selectViz(result.meta) 渲染 tab
    }
    class sqlBuilder {
        <<module>>
        +buildDefaultMetricSql(table)
        +buildSqlFromField/Table()
        +buildSpanSql(table)
    }
    QueryPanel --> SqlGraph
    SqlGraph ..> VizRegistry : selectViz
    SqlGraph ..> sqlBuilder

    %% ====== 数据契约 ======
    class SqlQueryResult {
        +columns
        +rows
        +meta time_column / dur_column
        +meta dimension_columns / value_columns
    }
    class SchemaTable {
        +name, columns
        +parent_table, join_key
        +dimension_keys, default_metrics
        +table_kind
    }
    SqlGraph ..> SqlQueryResult
    SqlGraph ..> SchemaTable
    VizDefinition ..> SqlQueryResult : accepts(meta)

    %% ====== 页面 ======
    class ExplorerPage {
        +选择数据源
        +plugin.QueryPanel 分发
    }
    ExplorerPage ..> PluginRegistry
    ExplorerPage --> QueryPanel
```

---

## 四、前端分层架构图

```mermaid
flowchart TB
    subgraph PAGES["页面层"]
        EXP["ExplorerPage 数据探索"]
        DSP["DatasourcePage 数据源管理"]
    end

    subgraph PLUGINS["数据源插件层 (plugins/)"]
        PREG["PluginRegistry getPlugin()"]
        FORMS["DatasourceForm (各格式)"]
        QPS["QueryPanel (各格式)"]
    end

    subgraph QUERY["查询与构建 (SqlGraph)"]
        SG["SqlGraph"]
        SB["sqlBuilder (含 span)"]
    end

    subgraph VIZ["可视化层 (visualizations/)"]
        VREG["VizRegistry selectViz(meta)"]
        TS["TimeseriesViz 折线"]
        TL["TimelineViz 泳道"]
        TB["TableViz 表格"]
    end

    subgraph SVC["服务层 (services/)"]
        SDS["datasource"]
        SSQL["sql"]
        SINS["inspectSource"]
    end

    API[("后端 REST API")]

    EXP --> PREG --> QPS --> SG
    DSP --> FORMS
    FORMS --> SINS
    SG --> SB
    SG --> VREG --> TS & TL & TB
    SG --> SSQL
    EXP --> SDS
    SDS & SSQL & SINS --> API
```

---

## 五、端到端流程图（建源 → 导入 → 查询 → 可视化）

```mermaid
sequenceDiagram
    autonumber
    actor U as 用户
    participant FE as 前端 (Form/Explorer)
    participant API as FastAPI (main.py)
    participant FR as FormatImporterRegistry
    participant IMP as Importer (mcap/proto/ctf)
    participant DB as DuckDB (+_pv_table_meta)
    participant VR as VizRegistry

    Note over U,DB: ① 配置并导入数据源
    U->>FE: 填写路径/类型, 点击扫描
    FE->>API: GET /api/inspect?plugin_type&path
    API->>FR: get(plugin_type)
    FR->>IMP: inspect(path)
    IMP-->>FE: topics / 区间信息

    U->>FE: 保存数据源
    FE->>API: POST /api/datasources
    API->>API: _try_ingest(ds_id) (capability=ingest)
    API->>FR: get(plugin_type)
    FR->>IMP: ingest(db_path, settings)
    IMP->>DB: 建表 + 写行 + write_table_meta(table_kind/dims)
    IMP-->>API: {messages_decoded/spans}
    API-->>FE: ingest_status = ready

    Note over U,VR: ② 查询并展示
    U->>FE: 进入 Explorer, 选数据源
    FE->>API: GET /api/datasources/{id}/schema
    API->>DB: get_schema (读 _pv_table_meta)
    DB-->>FE: tables[] (含 dimension_keys/table_kind)
    FE->>FE: sqlBuilder 生成预设 SQL (时序/span)
    U->>FE: 点击预设 / 编辑 SQL, 执行
    FE->>API: POST /api/sql/query
    API->>DB: run_query + 列语义推断
    DB-->>API: columns/rows/meta(time,dur,dims,values)
    API-->>FE: SqlQueryResult
    FE->>VR: selectViz(meta)
    VR-->>FE: [timeseries|timeline|table]
    FE-->>U: 渲染对应图表 (折线 / 泳道 / 表格)
```

---

## 六、数据形态 → 默认可视化对照

```mermaid
flowchart LR
    A["原始数据"] -->|MCAP/Protobuf| B["timeseries 表<br/>_time + 指标"]
    A -->|CTF tracing| C["span 表<br/>_time + _dur + track"]
    B -->|meta.value_columns| TS["TimeseriesViz 折线图"]
    C -->|meta.dur_column| TL["TimelineViz 泳道图"]
    B --> TBL["TableViz 表格"]
    C --> TBL
```

> 关键解耦点：可视化的选择只依赖**查询结果 `meta`**（是否有 `value_columns` / `dur_column`），
> 与数据来自哪种格式无关；新增格式只要落成对应 `table_kind` 的表即可自动复用图表。
