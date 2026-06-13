# system_stats — 前端诊断展示规范 🔧

此文档描述了位于 `src/diag` 下的 C++ 诊断项（CPU、文件系统、发布/订阅话题等）输出的字段含义及前端展示建议，面向中文前端开发人员，目标是简洁、易懂、适合普通用户的可视化展示（颜色提示、悬停显示详细信息、简明数值）。

---

## 1. 通用约定

- 后端每条诊断项使用 `diagnostic_msgs::msg::DiagnosticStatus`：
  - `name`（字符串）：诊断项类型，例如 `System CPU`、`Node CPU`、`Node Default CPU`、`File System`、`PubTopic`、`PubNodeTopic`、`SubTopic`、`SubNodeTopic`。
  - `level`（整数）：严重等级，映射为颜色：
    - `Diagnostics::ERROR` -> **红色**（严重）
    - `Diagnostics::WARN` -> **黄色**（警告）
    - 未设置或正常 -> 无需特殊颜色（视为正常）
  - `message`（字符串）：简短的可读描述，建议在鼠标悬停时显示完整信息。
  - `values`（KeyValue 数组）：携带用于展示的数值或元信息（例如 `name`、`value` 等）。

- 展示优先级规则：
  1. **System CPU**（如果存在）应放在最前并以更大视觉尺寸展示；其 `values` 仅包含一对 `value`（当前百分比）。
  2. 节点级 CPU（`Node CPU` 或 `Node Default CPU`）按节点显示，每个节点有两对 `values`：`name`（节点名）与 `value`（CPU 百分比字符串）。
  3. 文件系统及话题相关（Pub/Sub）可以分别放在独立面板内，使用相同的颜色语义。

---

## 2. CPU 诊断（来自 `cpu_diag_item.cpp`） 💡

- 后端可能产生的诊断项：
  - **System CPU**：
    - `values`: 单个 KeyValue，key=`value`，value 为当前 CPU 占用（字符串，例如 `"12.3"`）。
    - `message`: 如 `"System CPU usage is warn"` 或 `"... is error"`。
    - 前端展示：放在最上方并且字体/方块略大，始终以百分比形式展示（例如 `12.3%`）。
  - **Node CPU**（节点有特定阈值配置）：
    - `values`: 两个 KeyValue：`name`（节点名），`value`（CPU 占用字符串）。
    - `message`: 如 `"Node CPU usage is warn"` 或 `"... is error"`。
    - 前端展示：节点名 + 百分比，并根据 `level` 上色。
  - **Node Default CPU**（节点使用默认阈值）：
    - 与 `Node CPU` 相同的 `values` 结构，但 `status.name` 为 `Node Default CPU`。

- 前端交互与样式建议：
  - 颜色映射：WARN -> 黄色，ERROR -> 红色；正常使用中性色或不着色。
  - 悬停提示：展示 `message`（具体异常信息，用户悬停时可读）。
  - 排序：若 `System CPU` 处于 WARN/ERROR，请将其置顶并扩大显示。

---

## 3. 文件系统 (来自 `fs_diag_item.cpp`) 🗂️

- 诊断项：`File System`。
  - `values`：单个 KeyValue，key=`value`，value 为使用率字符串（例如 `"82.1"`）。
  - `message`：例如 `"FS /data used percent is 87.4%"`。

- 前端展示：显示挂载点与使用率，按 `level` 上色；鼠标悬停显示 `message`。

---

## 4. 发布/订阅话题诊断（来自 `pub_topic_diag_item.cpp` / `sub_topic_diag_item.cpp`）🔁

- 命名与含义：
  - 全局发布规则：`PubTopic`，`values: [{key: "value", value: "<hz>"}]`，`message` 会形如 `Topic <topic> is not published at the expected rate: <hz>hz`。
  - 节点特定发布：`PubNodeTopic`，`message` 包含节点名：`Node [<node>] Topic <topic> is not published ...`。
  - 订阅同理：全局 `SubTopic`，节点特定 `SubNodeTopic`。

- `values`:
  - 单个 KeyValue：key=`value`，value 为当前测得频率（hz）字符串，例如 `"12.0"`。

- 匹配逻辑（关键）：
  - 后端维护两个配置集：优先级更高的（node, topic）精确匹配与全局 topic 匹配（fallback）。前端无需自己做匹配逻辑，但应展示 `message` 中的信息，以便用户看出是否为节点特定规则。

- 前端展示：显示话题名、当前频率（hz）并按 `level` 上色，悬停显示 `message`（包含期望频率和节点信息）。

---

## 5. 前端渲染建议 ✅

- 总览布局：
  - 顶部行：系统关键项（`System CPU`），强调显示。
  - 下方：节点概览网格（每个节点一个卡片），显示节点 CPU 百分比与颜色状态。
  - 另外准备独立面板：文件系统、发布话题、订阅话题。

- 卡片内容：
  - 标题：类型或节点名（节点卡片使用 `name`）。
  - 主要内容：大号数字（CPU 百分比或 Hz），CPU 后加 `%`。
  - 颜色：WARN -> 黄，ERROR -> 红。正常无色或中性色。
  - 悬停：显示 `message` 详细信息（包含期望值、节点名等）。

- 无障碍提示：同时使用颜色与图标/文字（例如 ⚠️ 表示 WARN，❗ 表示 ERROR），便于色觉障碍用户识别。

---

## 6. 示例（伪 JSON，前端展示参考）

- System CPU（警告）：
```json
{
  "name": "System CPU",
  "level": "WARN",
  "message": "System CPU usage is warn",
  "values": [{"key": "value", "value": "72.5"}]
}
```

- Node CPU（严重）：
```json
{
  "name": "Node CPU",
  "level": "ERROR",
  "message": "Node CPU usage is error",
  "values": [{"key": "name", "value": "node_xyz"}, {"key": "value", "value": "92.1"}]
}
```

- 文件系统（警告）：
```json
{
  "name": "File System",
  "level": "WARN",
  "message": "FS /data used percent is 87.4%",
  "values": [{"key": "value", "value": "87.4"}]
}
```

- PubTopic（节点特定，警告）：
```json
{
  "name": "PubNodeTopic",
  "level": "WARN",
  "message": "Node [node_a] Topic /camera/image is not published at the expected rate: 30hz",
  "values": [{"key": "value", "value": "12.0"}]
}
```

---

## 7. 代码参考位置 🔎

- CPU: `src/diag/cpu_diag_item.cpp` / `cpu_diag_item.h`
- 文件系统: `src/diag/fs_diag_item.cpp` / `fs_diag_item.h`
- 发布话题: `src/diag/pub_topic_diag_item.cpp` / `pub_topic_diag_item.h`
- 订阅话题: `src/diag/sub_topic_diag_item.cpp` / `sub_topic_diag_item.h`

---

如果需要，我可以为你：

1. 添加一个示例页面（HTML/JS），它接收 `DiagnosticStatus` 数组并按上述规则渲染卡片；
2. 或者把示例转换成 JSON Schema / TypeScript 接口，便于前端直接引用。

请选择你要的下一步（例如“生成示例页面”或“提供 TypeScript 接口”），我来继续实现。 ✨