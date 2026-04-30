# Redis 缓存层优化开发文档

## TL;DR

> **目标**: 为 5 个延迟瓶颈模块加入 Redis 缓存层，采用 Cache-Aside 模式（优先命中缓存 → 回源 MySQL → 回填 Redis）
> **现状**: 题解列表已有部分缓存，但嵌套回复、点赞/收藏、评论发表完全无缓存
> **改动量**: 3 个文件（oj_model.hpp 核心），约 150 行新增

---

## Context

### 现有 Redis 架构

**Cache 类** (`oj_cache.hpp:22`):
```cpp
Cache(redis_addr, business="oj", env="prod", version="v2")
```

**Key 格式**: `oj:prod:v2:{type}:{entity}:{params}`

| 用途 | Key 示例 | TTL |
|------|----------|-----|
| 评论列表 | `comment:list:sid:{id}:page:{p}:size:{s}` | 已实现 |
| 用户统计 | `stats:user:{uid}` | 180±60s |
| 用户头像 | `avatar:user:{uid}` | 3600±600s |
| 题解列表 | `oj:prod:v2:list:...` (CacheListKey) | 600±120s |
| 题目详情 | `oj:prod:v2:static:...` | 21600±3600s |

**核心 API**:
- `_cache.SetStringByAnyKey(key, value, ttl_seconds)` — 写入
- `_cache.GetStringByAnyKey(key, &value)` — 读取
- `_cache.DeleteStringByAnyKey(key)` — 删除
- `_cache.BuildJitteredTtl(base, jitter)` — 防雪崩 TTL

### 5 个瓶颈模块现状

| 模块 | 当前缓存 | 延迟来源 |
|------|:---:|------|
| ① 评论列表加载 | ✅ 已有 (`comment:list:sid:...`) | 首加载 miss，及 N+1 reply_count 查询 |
| ② 嵌套回复加载 | ❌ 无 | 每次展开都查 MySQL |
| ③ 发表评论 | ❌ 无（仅 write-through 失效缓存） | MySQL INSERT + 缓存失效 |
| ④ 题解列表加载 | ✅ 已有 (CacheListKey) | 数据变更后失效重建 |
| ⑤ 点赞/收藏 | ❌ 无 | 每次操作直接 MySQL |

---

## Execution Strategy

```
Wave 1 (缓存添加 - 并行):
├── Task 1: 嵌套回复加载缓存 (GetCommentReplies)
├── Task 2: 点赞/收藏操作缓存 (ToggleCommentAction)
├── Task 3: 评论发表优化 (CreateComment 预写入缓存)

Wave 2 (缓存一致性):
├── Task 4: 评论删除时级联失效嵌套回复缓存
├── Task 5: 点赞/收藏后主动更新列表缓存中的计数

Wave 3:
└── Task 6: 构建验证 + 提交
```

---

## TODOs

### Task 1: 嵌套回复加载 — 加 Redis 缓存

**Key 格式**: `reply:list:pid:{parent_id}:page:{p}:size:{s}`

**TTL**: `BuildJitteredTtl(120, 30)` — 120±30s（回复变更频率低于顶级评论）

**实现** (`oj_model.hpp:GetCommentReplies`, line 1798):
- 在查询 MySQL 之前，先 `_cache.GetStringByAnyKey(cache_key, &cached_json)`
- Cache hit → 反序列化 JSON → 填充 `replies` vector 和 `total_count`
- Cache miss → 保持现有 MySQL 查询 → 序列化结果为 JSON → `_cache.SetStringByAnyKey(cache_key, json, ttl)`

**缓存失效触发点**:
- `CreateComment` 时：`_cache.DeleteStringByAnyKey("reply:list:pid:" + parent_id + ":page:1:size:50")`
- `DeleteComment` 删除回复时：同上
- `ToggleCommentAction` 时不失效（like_count 变化不影响回复列表结构，由前端实时更新）

**序列化格式**:
```json
{
  "total": 5,
  "comments": [
    {"id":15, "solution_id":1, "user_id":25, "content":"...",
     "is_edited":false, "parent_id":7, "reply_to_user_id":25,
     "like_count":1, "favorite_count":0, "created_at":"...",
     "updated_at":"...", "reply_to_user_name":"Yupureki"}
  ]
}
```

---

### Task 2: 点赞/收藏操作 — 加简单缓存 + 批量预热

**方案**: 点赞/收藏本身不缓存（操作必须落库），但：
1. 点赞/收藏后**主动更新**评论列表缓存中的 `like_count`/`favorite_count`（避免下次加载 miss）
2. 对 `comment_actions` 表查询加 `action:user:{uid}:comment:{cid}` 缓存（TTL 300±60s），避免每次统计用户是否点过赞都查 MySQL

**Key 格式**: `action:user:{uid}:comment:{cid}`

**值**: JSON `{"like": true, "favorite": false}`

**TTL**: `BuildJitteredTtl(300, 60)`

**实现** (`oj_model.hpp:ToggleCommentAction`, line 2241):
- 操作前先读缓存获取当前状态（避免重复 toggle 无效请求）
- 操作后更新缓存 `action:user:{uid}:comment:{cid}`（标记 liked/favorited）
- 同时更新评论列表缓存中对应评论的 like_count/favorite_count

**缓存失效**: 无（短 TTL 自动过期，操作后立即更新）

---

### Task 3: 评论发表 — 主动预热缓存

**方案**: 发表评论后，不等下次请求缓存 miss，直接**将新评论写入对应评论列表缓存的头部**。

**实现** (`oj_model.hpp:CreateComment`, line 1612):
- 插入 MySQL 成功后，尝试读取 `comment:list:sid:{sid}:page:1:size:20` 缓存
- 如果命中，将新评论 prepend 到 JSON 数组头部，重新写入缓存（TTL 不变）
- 如果未命中，不做额外操作（等下次请求正常 miss → fill）

**注意**: 顶级评论和回复的缓存 key 不同：
- 顶级评论: `comment:list:sid:{solution_id}:page:1:size:20`
- 嵌套回复: `reply:list:pid:{parent_id}:page:1:size:50`

---

### Task 4: 评论删除 — 级联失效嵌套回复缓存

**实现** (`oj_model.hpp:DeleteComment`, line 1904):
- 删除评论时，如果被删除的评论有 `parent_id > 0`（是回复），失效其父评论的回复缓存: `_cache.DeleteStringByAnyKey("reply:list:pid:" + parent_id + ":page:1:size:50")`
- 如果是顶级评论，失效其自身的回复缓存: `_cache.DeleteStringByAnyKey("reply:list:pid:" + comment_id + ":page:1:size:50")`
- 同时失效列表缓存（已有逻辑）

---

### Task 5: 点赞/收藏后更新列表计数缓存

当用户点赞/收藏时，如果评论列表缓存 `comment:list:sid:{sid}:page:1:size:20` 命中，直接在 JSON 中更新对应评论的 `like_count`/`favorite_count`，然后回写缓存，避免下次加载整个列表 miss。

**实现**: 在 `ToggleCommentAction` 成功后：
1. 尝试读取评论列表缓存（已知 solution_id 可从评论 ID 反查）
2. 遍历 JSON 找到对应评论
3. 更新 `like_count`/`favorite_count`
4. 回写缓存（保持原 TTL）

---

## Cache Key 汇总

| Key | 格式 | TTL | 用途 |
|-----|------|-----|------|
| 评论列表 | `comment:list:sid:{id}:page:{p}:size:{s}` | 已存在 | 顶级评论列表 |
| **回复列表** | **`reply:list:pid:{pid}:page:{p}:size:{s}`** | **120±30s** | **嵌套回复列表（新增）** |
| **用户动作** | **`action:user:{uid}:comment:{cid}`** | **300±60s** | **点赞/收藏状态（新增）** |
| 用户统计 | `stats:user:{uid}` | 180±60s | 已解决/提交统计 |
| 用户头像 | `avatar:user:{uid}` | 3600±600s | 头像路径 |

---

## 缓存失效矩阵

| 操作 | 失效的缓存 Key |
|------|---------------|
| 发表顶级评论 | `comment:list:sid:{sid}:page:1:size:*`（已实现） |
| **发表回复** | `comment:list:sid:{sid}:page:1:size:*`（已实现）+ **`reply:list:pid:{pid}:page:1:size:50`（新增）** |
| 删除评论 | `comment:list:sid:{sid}:page:1:size:*`（已实现）+ `reply:list:pid:{pid}:page:1:size:50`（已实现）+ 题解列表缓存（已实现） |
| **点赞/收藏** | **`action:user:{uid}:comment:{cid}`（新增，删除旧值后重写）** |
| 编辑评论 | `comment:list:sid:{sid}:page:1:size:*`（已实现？待确认） |

---

## Verification

部署后对比缓存命中率：
```bash
redis-cli INFO stats | grep keyspace
redis-cli MONITOR | grep "comment\|reply\|action"
```

预期：重复展开同一评论的回复列表时，第二次开始命中缓存，延迟降至 <5ms。
