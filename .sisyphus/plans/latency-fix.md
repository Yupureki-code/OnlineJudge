# 延迟优化开发文档

## TL;DR

> **目标**: 消除题解/评论系统的 N+1 查询、增强缓存、减少往返，将关键路径延迟降低 50-80%
> **核心瓶颈**: 5 个 N+1 循环查询，3 个缺失批量化，4 个可合并查询
> **改动量**: `oj_model.hpp` + `oj_control.hpp`，约 100 行变更

---

## 瓶颈分析

### 慢 #1: 评论列表加载 — N+1 × 2

**GetTopLevelComments**（oj_control.hpp:1264）每条评论 2 次额外 DB 查询：

| 行号 | 查询 | 调用 |
|------|------|------|
| 1250 | `GetUserById(user_id)` | 每个评论查作者名/头像 |
| 1321 | `SELECT count(*) WHERE parent_id=` | 每个评论查回复数 |

**影响**: 20 条一级评论 = 1（列表查询）+ 20×2 = **41 次 DB 查询**

**优化**:
- `GetUserById` 批量化：在 `GetCommentsBySolutionId` model 中 JOIN 用户表，一次性获取 author_name/author_avatar，无需逐个查
- `reply_count` 批量化：用 `SELECT parent_id, count(*) FROM solution_comments WHERE parent_id IN (...) GROUP BY parent_id` 一次获取所有计数

---

### 慢 #2: 嵌套回复加载 — N+1 × 2

**GetCommentReplies**（oj_control.hpp:1332）每条回复 2 次额外 DB 查询：

| 行号 | 查询 | 调用 |
|------|------|------|
| 1386 | `GetUserById(r.user_id)` | 每个回复查作者 |
| 1390+ | `GetCommentReplies(r.id, ...)` | 每个回复查嵌套回复数 |

**影响**: 10 条回复 = 10 + 10×2 = **31 次 DB 查询**

**优化**:
- `GetUserById` 批量化：同 #1，model 中 JOIN users 表
- `reply_count` 批量化：同 #1，GROUP BY parent_id 批量获取

---

### 慢 #3: 题解列表 — N+1

**GetSolutionList**（oj_control.hpp:1578）：

| 行号 | 查询 | 调用 |
|------|------|------|
| 1578 | `GetUserById(s.user_id)` | 每个题解查作者 |

**优化**: 在 model 中 JOIN users 表批量获取。

---

### 慢 #4: 评论发布/删除 — 多次往返

**CreateComment**（oj_model.hpp:1612）：

| 步骤 | 查询 |
|------|------|
| 1 | `GetSolutionById` — 验证题解存在 |
| 2 | `GetCommentById(parent_id)` — 验证父评论（当 parent_id>0） |
| 3 | INSERT comment |
| 4 | UPDATE solutions comment_count |
| 5 | Invalidate 评论列表缓存 (3 keys) |
| 6 | Invalidate 回复缓存 (3 keys) |
| 7 | Invalidate 题解列表缓存 |
| 8 | `GetCommentById(new_id)` — 获取完整评论（controller 层） |
| 9 | `GetUserById(author_id)` — 获取作者信息（controller 层） |

**8-9 步合并优化**: 第 8 步 `GetCommentById` 的 SQL 已 JOIN users 表（获取 author_name、reply_to_user_name），controller 不需要再单独查 `GetUserById`。

**DeleteComment**（oj_model.hpp:2031）：

| 步骤 | 查询 |
|------|------|
| 1 | `SELECT solution_id,parent_id` — 获取所属题解 |
| 2 | `SELECT count(*) WHERE parent_id=` — 统计子评论数 |
| 3 | DELETE 子评论 |
| 4 | DELETE 主评论 |
| 5 | `SELECT question_id FROM solutions` — 获取题目 ID |
| 6 | UPDATE solutions comment_count |
| 7 | Invalidate 题解列表缓存 |

**1+5 合并**: 步骤 1 和 5 查询同一个 solutions 表，合并为一次 `SELECT solution_id, parent_id, question_id FROM solutions WHERE id=`。

---

### 慢 #5: Controller 层冗余调用

**GetTopLevelComments** 中重复查作者：
- 行 1250: `GetUserById(c.user_id)` — 查作者
- 行 1308: `GetUserById(c.user_id)` — 同样查作者（不同的 if/else 分支）

**RunSingleTest** 重复查题目：
- 行 2311: `GetOneQuestion(question_id, q)` — 在 `RunSingleTest` 中
- `Judge` 函数内部的 `GetOneQuestion` — 同一函数被 `RunSingleTest` 调用，重复查询

---

## 改进计划

### Wave 1: 批量查询（消除 N+1）

| 任务 | 文件 | 内容 |
|------|------|------|
| Task 1 | `oj_model.hpp` | `GetCommentsBySolutionId` 加 LEFT JOIN users，直接返回 author_name/author_avatar |
| Task 2 | `oj_model.hpp` | 新增 `GetReplyCounts(vector<id>)` 批量方法，用 `GROUP BY parent_id` 一次获取所有计数 |
| Task 3 | `oj_model.hpp` | `GetCommentReplies` 加 LEFT JOIN users |
| Task 4 | `oj_model.hpp` | 题解列表加 LEFT JOIN users |

### Wave 2: 减少往返

| 任务 | 文件 | 内容 |
|------|------|------|
| Task 5 | `oj_control.hpp` | `GetTopLevelComments`: 用批量 reply_count 替代逐条 COUNT |
| Task 6 | `oj_control.hpp` | `GetCommentReplies`: 用批量 reply_count + 移除逐条 GetUserById |
| Task 7 | `oj_model.hpp` | `DeleteComment`: 合并 solution_id + question_id 查询 |
| Task 8 | `oj_control.hpp` | `PostComment` 响应构建: 复用 INSERT 后的数据，移除 GetCommentById 重复查询 |

### Wave 3: 缓存优化

| 任务 | 文件 | 内容 |
|------|------|------|
| Task 9 | `oj_model.hpp` | 增加 comment author 缓存（`author:user:{uid}`），减少 JOIN 查询 |
| Task 10 | `oj_model.hpp` | 评论列表缓存 TTL 从 300s 延长到 600s，失效更精确（只删变更页） |

### 预期效果

| 场景 | 优化前 | 优化后 | 改善 |
|------|--------|--------|------|
| 加载 20 条评论 | ~41 次 DB 查询 | ~3 次 DB 查询 | **-93%** |
| 加载 10 条回复 | ~31 次 DB 查询 | ~2 次 DB 查询 | **-94%** |
| 发布评论 | 9 次 DB 操作 | 5 次 DB 操作 | **-44%** |
| 删除评论 | 7 次 DB 操作 | 5 次 DB 操作 | **-29%** |
| 题解列表 10 条 | ~11 次 DB 查询 | ~1 次 DB 查询 | **-91%** |
