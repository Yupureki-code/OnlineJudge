# 题目写接口文档

更新时间：2026-04-21

本文档描述题目管理写路径相关接口，包含鉴权要求、请求格式、返回字段及缓存一致性行为。

## 1. 总览

- 服务地址：`http://127.0.0.1:8080`
- 内容类型：`application/json`
- 认证方式：登录态 Cookie（Session）
- 未登录返回：`401`

接口清单：
- `POST /api/questions`：新增或更新题目（Upsert）
- `DELETE /api/question/:id`：删除题目
- `POST /api/questions/cache/invalidate`：手动递增列表版本（缓存逻辑失效）

## 2. 缓存一致性约定

写接口成功后会自动执行以下动作：

1. 题目保存：刷新对应题目详情缓存。
2. 题目删除：写入空值缓存，避免穿透。
3. 新增/更新/删除成功后：自动递增 `question:list:version`，使题目列表分页缓存逻辑失效。

说明：
- 列表缓存采用带版本 key 的策略，版本递增后旧版本缓存不再命中。
- 该策略用于保障 `GET /all_questions` 不读取旧列表。

## 3. 鉴权与 CORS

- 写接口需要登录态，未登录返回 `401`。
- 已支持预检请求：
  - `OPTIONS /api/questions`
  - `OPTIONS /api/question/:id`
  - `OPTIONS /api/questions/cache/invalidate`
- `Access-Control-Allow-Methods`：`POST, GET, DELETE, OPTIONS`

## 4. 接口详情

### 4.1 新增或更新题目

- 方法与路径：`POST /api/questions`
- 说明：
  - 当 `number` 不存在时插入新题目。
  - 当 `number` 已存在时更新该题目。

请求体：

```json
{
  "number": "1001",
  "title": "两数之和",
  "desc": "给定整数数组，返回和为目标值的两个下标。",
  "star": "简单",
  "cpu_limit": 1,
  "memory_limit": 128
}
```

字段约束：
- `number`：字符串，必须为纯数字。
- `title`：字符串，不能为空。
- `desc`：字符串。
- `star`：字符串。
- `cpu_limit`：整数。
- `memory_limit`：整数。

成功响应（200）：

```json
{
  "success": true,
  "question_id": "1001"
}
```

失败响应：
- 401（未登录）

```json
{
  "success": false,
  "error": "未登录"
}
```

- 400（请求体或字段错误）

```json
{
  "success": false,
  "error": "缺少必要字段(number/title/desc/star/cpu_limit/memory_limit)"
}
```

- 500（数据库写失败等）

```json
{
  "success": false,
  "error": "题目写入失败"
}
```

### 4.2 删除题目

- 方法与路径：`DELETE /api/question/:id`
- 路径参数：
  - `id`：题号，数字字符串。

成功响应（200）：

```json
{
  "success": true,
  "question_id": "1001"
}
```

失败响应：
- 401（未登录）

```json
{
  "success": false,
  "error": "未登录"
}
```

- 500（删除失败）

```json
{
  "success": false,
  "error": "题目删除失败"
}
```

### 4.3 手动失效题目列表缓存

- 方法与路径：`POST /api/questions/cache/invalidate`
- 说明：手动触发列表版本递增，通常用于运维或调试。

成功响应（200）：

```json
{
  "success": true,
  "list_version": "12",
  "operator": "user@example.com"
}
```

失败响应：
- 401（未登录）

```json
{
  "success": false,
  "error": "未登录"
}
```

## 5. 联调示例

### 5.1 新增或更新题目

```bash
curl -i -X POST 'http://127.0.0.1:8080/api/questions' \
  -H 'Content-Type: application/json' \
  -H 'Cookie: SESSION_ID=<你的会话ID>' \
  -d '{
    "number":"1001",
    "title":"两数之和",
    "desc":"给定整数数组，返回和为目标值的两个下标。",
    "star":"简单",
    "cpu_limit":1,
    "memory_limit":128
  }'
```

### 5.2 删除题目

```bash
curl -i -X DELETE 'http://127.0.0.1:8080/api/question/1001' \
  -H 'Cookie: SESSION_ID=<你的会话ID>'
```

### 5.3 手动失效列表缓存

```bash
curl -i -X POST 'http://127.0.0.1:8080/api/questions/cache/invalidate' \
  -H 'Cookie: SESSION_ID=<你的会话ID>'
```

## 6. 现阶段限制

- 当前仅做“登录态校验”，尚未引入管理员角色权限控制。
- 建议下一步补充：
  - 管理员鉴权（仅管理员可写）
  - 写操作审计日志（操作者、时间、变更前后）
  - 批量题目导入与回滚机制
