# OnlineJudge 开发详细文档

> 基于 OMNI.md 项目规范 + plan.md 需求 + 全量代码扫描生成

---

## 一、模块功能梳理

### 1.1 系统架构总览

```
┌─────────────┐     HTTP      ┌──────────────┐    HTTP/JSON     ┌─────────────────┐
│  浏览器/SPA  │ ───────────→ │   oj_server   │ ──────────────→  │ compile_server  │
│  (前端)      │ ←─────────── │   :8080       │ ←──────────────  │   :8081+       │
└─────────────┘               ├──────────────┤                   └─────────────────┘
                              │  oj_admin    │
                              │   :8090      │
                              └──────┬───────┘
                                     │
                        ┌────────────┼────────────┐
                        │            │            │
                   ┌────┴───┐  ┌────┴────┐  ┌───┴─────┐
                   │ MySQL  │  │  Redis  │  │ Logger  │
                   └────────┘  └─────────┘  └─────────┘
```

### 1.2 oj_server 模块职责

**MVC 架构**：

| 模块 | 文件 | 职责 |
|------|------|------|
| **Model** | `oj_model.hpp` | MySQL/Redis 数据操作，缓存管理，用户/题解/评论 CRUD |
| **View** | `oj_view.hpp` | ctemplate HTML 渲染（题目列表、题目详情） |
| **Controller** | `oj_control.hpp` | 业务逻辑：路由处理、判题调度、认证、题解、评论 |
| **Session** | `oj_session.hpp` | Redis + 本地 map 双写会话管理 |
| **Cache** | `oj_cache.hpp` | 三级缓存（Redis → View 内存 → 磁盘），Key 抽象体系 |
| **Mail** | `oj_mail.hpp` | SMTP 邮件发送（libcurl），验证码邮件 |
| **路由入口** | `oj_server.cpp` | httplib 路由注册，HTTP 请求分发，参数解析 |
| **公共工具** | `comm.hpp` | User/Question/AdminAccount/Comment 等结构体，工具函数 |
| **配置** | `config.in.h` → `config.h` | CMake 变量 + 环境变量 fallback |

**路由表**（完整，按功能分类）：

| 分类 | 方法 | 路由 | 处理函数 | 说明 |
|------|------|------|----------|------|
| **页面** | GET | `/` | ctl.GetStaticHtml("index.html") | 首页 |
| | GET | `/all_questions` | ctl.AllQuestions | 题目列表 SSR |
| | GET | `/questions/(\d+)` | ctl.OneQuestion | 题目详情 SSR |
| | GET | `/about` | GetStaticHtml | 关于页 |
| | GET | `/user/profile` | GetStaticHtml | 个人中心 |
| | GET | `/user/settings` | GetStaticHtml | 账户设置 |
| | GET | `/questions/(\d+)/solutions/new` | GetStaticHtml | 发布题解页 |
| | GET | `/solutions/(\d+)` | GetStaticHtml | 题解详情页 |
| | GET | `/judge_result.html` | GetStaticHtml | 判题结果页 |
| **认证** | POST | `/api/auth/send_code` | SendEmailAuthCode | 发验证码 |
| | POST | `/api/auth/verify_code` | VerifyEmailAuthCodeAndLogin | 验证码登录/注册 |
| | POST | `/api/user/password/login` | LoginWithPassword | 密码登录 |
| | POST | `/api/user/logout` | DestroySessionByCookieHeader | 登出 |
| | POST | `/api/user/security/send_code` | SendEmailAuthCode | 安全操作验证码 |
| | POST | `/api/user/email/change` | ChangeEmailWithCode | 改邮箱 |
| | POST | `/api/user/password/change` | ChangePasswordWithCode | 改密码 |
| | POST | `/api/user/account/delete` | DeleteAccountWithCode | 注销账号 |
| | POST | `/api/user/password/set` | SetPasswordForUser | 设密码 |
| **用户** | POST | `/api/user/check` | CheckUser | 检查用户存在 |
| | POST | `/api/user/create` | CreateUser | 创建用户 |
| | POST | `/api/user/get` | GetUser | 获取用户 |
| | GET | `/api/user/info` | GetSessionUser | 当前登录用户信息 |
| **判题** | POST | `/judge/(\d+)` | Judge | 提交代码判题（表单POST） |
| **题目API** | GET | `/api/question/(\d+)` | GetOneQuestion | 题目详情 JSON |
| | GET | `/api/question/(\d+)/pass_status` | HasUserPassedQuestion | 是否通过 |
| | POST | `/api/questions` | SaveQuestion | 新增/更新题目 |
| | DELETE | `/api/question/(\d+)` | DeleteQuestion | 删除题目 |
| | POST | `/api/questions/cache/invalidate` | TouchQuestionListVersion | 刷新题目列表缓存 |
| | POST | `/api/static/cache/invalidate` | InvalidateStaticHtmlCache | 刷新静态页缓存 |
| | GET | `/api/metrics/cache` | 缓存指标 | 缓存命中率统计 |
| **题解** | GET | `/api/questions/(\d+)/solutions` | GetSolutionList | 题解列表 |
| | POST | `/api/questions/(\d+)/solutions` | PublishSolution | 发布题解 |
| | GET | `/api/solutions/(\d+)` | GetSolutionDetail | 题解详情 |
| | POST | `/api/solutions/(\d+)/like` | ToggleLike | 点赞 |
| | POST | `/api/solutions/(\d+)/favorite` | ToggleFavorite | 收藏 |
| | GET | `/api/solutions/(\d+)/actions` | GetUserSolutionActions | 当前用户点赞/收藏状态 |
| **评论** | GET | `/api/solutions/(\d+)/comments` | GetComments | 评论列表 |
| | POST | `/api/solutions/(\d+)/comments` | PostComment | 发评论 |
| | PUT | `/api/comments/(\d+)` | EditComment | 编辑评论 |
| | DELETE | `/api/comments/(\d+)` | DeleteComment | 删除评论 |

### 1.3 oj_admin 模块职责

独立 HTTP 服务（:8090），独立 admin-session管理体系：
- Cookie: `oj_admin_session_id`，Redis key `oj:prod:v1:session:admin:`，1小时 TTL
- 角色: `super_admin`, `admin`
- 审计日志: `admin_audit_logs` 表

| 路由 | 功能 |
|------|------|
| `POST /api/admin/auth/login` | 管理员登录 |
| `POST /api/admin/auth/logout` | 管理员登出 |
| `GET /api/admin/overview` | 仪表盘统计 |
| `GET /api/admin/users` | 用户列表 |
| `GET /api/admin/questions` | 题目列表 |
| CRUD `/api/admin/questions/:number` | 题目增删改查 |

### 1.4 compile_server 模块职责

责任链模式：`HandlerPreProcesser → HandlerCompiler → HandlerRunner → HandlerJudge`

- 端口由命令行参数决定
- `POST /compile_and_run` 接收 JSON `{id, code}`，返回判题结果 JSON
- 每个 handler 处理一步：源文件生成 → 编译 → 运行 → 判题
- 不需要 Redis/MySQL，独立部署

### 1.5 数据库现状

| 表名 | 用途 | 关键字段 |
|------|------|----------|
| `users` | 用户 | uid, name, email, password_hash, password_algo |
| `questions` | 题目 | id(varchar5), title, desc, star, cpu_limit, mem_limit |
| `tests` | 测试用例 | id, question_id, `in`, `out`, is_sample |
| `solutions` | 题解 | id, question_id, user_id, title, content_md, like/favorite/comment_count, status |
| `solution_actions` | 题解互动 | id, solution_id, user_id, action_type(like/comment/favorite) |
| `solution_comments` | 题解评论 | id, solution_id, user_id, content, is_edited, created_at, updated_at |
| `user_submits` | 提交记录 | user_id, question_id, result_json, is_pass, action_time |
| `admin_accounts` | 管理员 | admin_id, password_hash, uid, role |
| `admin_audit_logs` | 审计日志 | log_id, request_id, operator_admin_id, action, resource_type, result |

### 1.6 Redis 缓存体系

Key 前缀规范：`oj:prod:v{version}:{type}:{kind}:...`

| Key 模式 | TTL | 用途 |
|----------|-----|------|
| `oj:prod:v1:session:user:{id}` | 7天 | 用户会话 |
| `oj:prod:v1:session:admin:{id}` | 1小时 | 管理员会话 |
| `oj:prod:v1/v2:list:question:...` | - | 题目列表缓存 |
| `oj:prod:v1/v2:detail:data:...` | - | 题目数据缓存 |
| `oj:prod:v1/v2:static:html:...` | - | 整页HTML缓存 |
| `oj:auth:*` | 5分钟/1天 | 验证码、限流 |

---

## 二、需求分析与开发计划

### Bug #1: 题解评论删除段错误

**现象**：用户在题解详情页删除自己的评论时，oj_server 进程崩溃

**崩溃日志**：
```
[2026-04-28 15:17:28][INFO][60963][oj_cache.hpp:658] - Html page oj:prod:v2:static:html:solutions/detail.html cache hit
[2026-04-28 15:17:28][INFO][60963][oj_control.hpp:1578] - [html_cache][static] hit=1 page=solutions/detail.html source=redis
段错误 (核心已转储)
```

**调用链定位**：
```
DELETE /api/comments/(\d+)
  → oj_server.cpp:1299  svr.Delete handler
    → std::stoll(req.matches[1])
    → ctl.DeleteComment(comment_id, current_user, &result, &err_code)
      → Control::DeleteComment (oj_control.hpp:1277)
        → Model::GetCommentById(comment_id, &c)        // 查评论
        → Model::GetAdminByUid(current_user.uid)          // 查管理员
        → Model::DeleteComment(comment_id, uid, is_admin) // 删除
          → mysql_query(SELECT solution_id ...)            // 查所属题解
          → std::stoull(row[0])                           // ⚠️ 可能崩溃点
          → mysql_query(DELETE FROM solution_comments ...)  // 删除评论
          → ExecuteSql(UPDATE solutions SET comment_count...) // 更新计数
```

**根因分析（按概率排序）**：

1. **`std::stoull(row[0])` 异常崩溃**（最高概率）：
   - `Model::DeleteComment:1500` 中 `solution_id = std::stoull(row[0])`
   - 如果 `row[0]` 为空字符串、包含非数字字符、或超出 `unsigned long long` 范围，`std::stoull` 抛出 `std::invalid_argument` 或 `std::out_of_range`
   - 函数无 try-catch → `std::terminate()` → 进程崩溃
   - 同理 `DeleteComment` 中 `std::stoll(req.matches[1])` 也无保护

2. **未调用 `mysql_library_init()`** 导致线程不安全：
   - `httplib::Server` 默认使用线程池处理并发请求
   - MySQL C API 要求多线程环境先调用 `mysql_library_init()`
   - 当前代码 `main()` 中无此调用，并发 MySQL 操作可能段错误

3. **`mysql_store_result` 返回 NULL 未正确处理**：

**修复方案**：
- **最高优先**：`Model::DeleteComment` 和所有 `std::stoll/std::stoull` 调用处加 try-catch 保护
- **高优先**：`oj_server.cpp` 的 `main()` 开头添加 `mysql_library_init(0, nullptr, nullptr)`
- **中优先**：所有 `mysql_store_result` 返回 nullptr 的路径加防护并记录详细日志
- **建议**：编译时添加 `-fsanitize=address` 运行，精确定位崩溃点

**涉及文件**：
- `src/oj_server/src/oj_server.cpp` — main() 添加 mysql_library_init；DELETE/PUT handler 加 try-catch
- `src/oj_server/include/oj_model.hpp` — DeleteComment 中 stoull 加保护；所有 MySQL 结果集处理加防护
- `src/oj_server/include/oj_control.hpp` — DeleteComment 方法无关改动（确认逻辑无误）

---

### Feature 2.1: 用户头像自定义

**现状**：`User` 结构体无 avatar 字段，前端 `user/profile.html` 显示默认头像

**设计决策**：文件系统存储 + DB 路径（用户选择方案二）

**实现层次**：

#### 数据库变更
```sql
-- docs/db_migrations/20260428_0001_add_user_avatar.sql
ALTER TABLE users ADD COLUMN avatar_path VARCHAR(255) DEFAULT '' COMMENT '用户头像路径' AFTER email;
```

#### 后端修改

| 文件 | 修改内容 |
|------|----------|
| `src/comm/comm.hpp` | `User` 结构体增加 `std::string avatar_path` 字段 |
| `src/oj_server/include/oj_model.hpp` | `QueryUser` 查询增加 `avatar_path` 字段；新增 `UpdateUserAvatar(uid, path)` 方法；头像文件保存到 `HTML_PATH + "pictures/avatars/"` |
| `src/oj_server/include/oj_control.hpp` | 新增 `UploadAvatar(user, file_data, content_type)` 方法 |
| `src/oj_server/src/oj_server.cpp` | 新增路由 `POST /api/user/avatar`，使用 httplib 的 `MultipartFormData` 接收上传；设置静态目录 `svr.set_mount_point("/pictures/avatars", ...)` |
| `config.in.h` | 新增 `AVATAR_MAX_SIZE` 配置（默认 2MB） |

**头像存储流程**：
1. 前端 POST `/api/user/avatar`（multipart/form-data）
2. 后端校验文件大小 ≤ 2MB，类型为 jpg/png/gif/webp
3. 生成文件名 `avatars/{uid}_{timestamp}.{ext}`
4. 保存到 `HTML_PATH/pictures/avatars/` 目录
5. 更新 `users.avatar_path` 为 `/pictures/avatars/{filename}`
6. 返回 JSON `{success: true, avatar_url: "/pictures/avatars/{filename}"}`

#### 前端修改

| 文件 | 修改内容 |
|------|----------|
| `src/wwwroot/user/profile.html` | 头像区域增加点击上传功能，添加 `<input type="file">` 隐藏框 |
| `src/wwwroot/js/auth.js` | `SERVER_USER_INFO` 增加 `avatar_url` 字段注入 |
| 各 HTML 页面 | 导航栏/用户名旁边显示头像 `<img>` |

**API 设计**：
- `POST /api/user/avatar` — 上传头像（multipart/form-data，需登录）
- `GET /api/user/info` — 响应增加 `avatar_url` 字段
- `DELETE /api/user/avatar` — 删除头像（恢复默认）

---

### Feature 2.2: 题解详情页内嵌切换

**现状**：`solutions/detail.html` 作为独立页面，点击题解跳转新 URL `/solutions/{id}`

**要求**：点击题解时，题解列表区域替换为详情内容，不跳转页面。同时保留 `solutions/detail.html` 独立页作为直接访问入口

**涉及文件**：
- `src/wwwroot/one_question.html` — 题目详情页（当前题解列表在此页内展示）
- `solutions/detail.html` — 题解详情独立页（保留）

**实现方案**：

1. 题目详情页 `one_question.html` 的题解列表区域给一个容器 `<div id="solution-content-area">`
2. 点击题解时，JS 通过 `fetch('/api/solutions/{id}')` 获取详情 JSON
3. 将题解详情渲染到该容器中（替换列表），同时 `history.pushState` 更新 URL（不刷新）
4. 提供"返回列表"按钮，点击后 `history.back()` 恢复题解列表
5. 点赞/收藏/评论功能全部在详情视图中工作（已有后端 API）
6. `solutions/detail.html` 独立页保留作为直接访问的备选入口

**注意点**：
- 已有 `/api/solutions/{id}` JSON API 返回题解详情
- 已有 `/api/solutions/{id}/like`, `/api/solutions/{id}/favorite`, `/api/solutions/{id}/actions` API
- 已有评论 CRUD API
- 前端改动集中在 `one_question.html` 的内联 JS

---

### Feature 2.3: 点赞/收藏功能

**现状**：后端 API 完整（`ToggleLike`, `ToggleFavorite`, `GetUserSolutionActions`），前端按钮未连接

**需要做的工作**：

1. **`solutions/detail.html`** — 确认点赞/收藏按钮绑定 `POST /api/solutions/{id}/like` 和 `POST /api/solutions/{id}/favorite`
2. **页面加载时** 调用 `GET /api/solutions/{id}/actions` 获取当前用户是否已点赞/收藏
3. **按钮状态切换** — 点击后：
   - POST like/favorite API
   - 根据返回的 `liked`/`favorited` 切换按钮高亮状态
   - 更新显示的 `like_count`/`favorite_count`
4. **`one_question.html`** 中题解卡片区 — 也需显示点赞/收藏数

**已有后端无需修改**，纯前端连接工作。

---

### Feature 2.4: 提交记录展示

**现状**：题目详情页有"提交记录"按钮，但无功能实现

**需要新增**：

#### 后端 API

**`GET /api/user/submits`**（需登录）

请求参数：
- `page`: 页码（默认1）
- `size`: 每页条数（默认20）

返回 JSON：
```json
{
  "success": true,
  "submits": [
    {
      "question_id": "1",
      "question_title": "A+B问题",
      "result_json": "...",
      "is_pass": true,
      "action_time": "2026-04-28 12:00:00"
    }
  ],
  "total": 50,
  "page": 1,
  "size": 20
}
```

**OR**（题目维度的版本）

**`GET /api/question/:id/submits`**（查看某题的提交记录）

请求参数：
- 无需分页（plan.md要求"超出界面则滚轮查看，不分页"）

返回 JSON：
```json
{
  "success": true,
  "submits": [
    {
      "submit_time": "2026-04-28 12:00:00",
      "result_json": "{ ... }",
      "is_pass": true
    }
  ]
}
```

#### 后端修改

| 文件 | 修改内容 |
|------|----------|
| `src/oj_server/include/oj_model.hpp` | 新增 `GetUserSubmitsByQuestion(uid, question_id, submits*)` 方法，查 `user_submits` 表 |
| `src/oj_server/include/oj_control.hpp` | 新增 `GetUserSubmits(question_id, user, result, err_code)` 方法 |
| `src/oj_server/src/oj_server.cpp` | 新增路由 `GET /api/question/{id}/submits` |

#### 前端修改

- `one_question.html` — "提交记录"按钮点击后，调用 API 并在下方区域展示列表

---

### Feature 2.5: 测试用例和测试结果

**现状**：题目详情页有"测试用例"/"测试结果"按钮，但无实现

**最复杂的功能**。分为两个子功能：

#### 2.5a: 测试用例展示

**后端 API**：

**`GET /api/question/:id/sample_tests`**（无需登录）

返回 JSON：
```json
{
  "success": true,
  "tests": [
    {"id": 1, "is_sample": true, "input": "1 2\n", "output": "3\n", "case_name": "case1"},
    {"id": 2, "is_sample": true, "input": "5 6\n", "output": "11\n", "case_name": "case2"}
  ]
}
```

**数据来源**：MySQL `tests` 表，`is_sample = 1` 的记录

#### 2.5b: 临时测试 / 正式测试

**关键设计决策**：不修改 compile_server 协议

**方案**：
- 前端发送用户代码 + 选择的测试输入到 oj_server
- oj_server 根据选择组装 JSON：
  - 默认测试用例：从数据库取该用例的 `in/out`，构建单个 test 的 JSON
  - 自定义测试用例：用户输入作为 input，取后台所有非 sample 测试用例作为验证
- oj_server 通过现有 `/compile_and_run` 接口发送给 compile_server
- 返回结果展示在"测试结果"区域

**后端修改**：

| 文件 | 修改内容 |
|------|----------|
| `src/oj_server/include/oj_model.hpp` | 新增 `GetTestsByQuestionId(question_id, tests*)` 获取所有测试用例 |
| `src/oj_server/include/oj_model.hpp` | 新增 `GetSampleTestsByQuestionId(question_id, tests*)` 获取 is_sample=1 的测试用例 |
| `src/oj_server/include/oj_control.hpp` | 新增 `RunSingleTest(question_id, code, test_input, test_output, user, result, err_code)` — 构建单用例JSON，调用Judge |
| `src/oj_server/src/oj_server.cpp` | 新增路由 `GET /api/question/{id}/sample_tests` 和 `POST /api/question/{id}/test` |

**测试 API 设计**：

`POST /api/question/{id}/test`

请求 JSON：
```json
{
  "code": "#include <iostream>...",
  "test_type": "sample",     // "sample" 或 "custom"
  "test_case_id": 1,         // sample 模式：默认测试用例 ID
  "custom_input": "5 6\n"    // custom 模式：用户自定义输入
}
```

返回 JSON（与判题结果格式一致）：
```json
{
  "status": "AC",
  "stdout": "11\n",
  "stderr": "",
  "desc": "答案正确",
  "input": "5 6\n",
  "expected_output": "11\n"
}
```

#### 前端改动

- `one_question.html` — 新增测试用例/测试结果选项卡 UI
- 默认测试用例以只读输入框展示（`is_sample=1` 的数据自动填入）
- 自定义用例有 "+" 按钮，生成空输入框
- "测试" 按钮发送 `POST /api/question/{id}/test`
- 测试结果下方展示输出、是否通过、错误原因

---

### Feature 2.6: 用户中心 - 提交统计

**现状**：个人中心有"最近提交"、"已解决题目"、"正确率"区域但未实现

**后端 API**：

**`GET /api/user/stats`**（需登录）

返回 JSON：
```json
{
  "success": true,
  "total_submits": 50,
  "passed_questions": 30,
  "accuracy": 0.6,
  "recent_submits": [
    {"question_id": "1", "title": "A+B问题", "star": "简单", "is_pass": true, "action_time": "2026-04-28 12:00:00"}
  ]
}
```

**SQL 查询思路**：
```sql
-- 总提交次数
SELECT COUNT(*) FROM user_submits WHERE user_id = ?;

-- 已通过的不同题目数
SELECT COUNT(DISTINCT question_id) FROM user_submits WHERE user_id = ? AND is_pass = 1;

-- 最近提交（最新20条）
SELECT us.question_id, us.is_pass, us.action_time, us.result_json,
       q.title, q.star
FROM user_submits us
LEFT JOIN questions q ON us.question_id = q.id
WHERE us.user_id = ?
ORDER BY us.action_time DESC
LIMIT 20;
```

**后端修改**：

| 文件 | 修改内容 |
|------|----------|
| `src/oj_server/include/oj_model.hpp` | 新增 `GetUserStats(uid, stats*)` 方法 |
| `src/oj_server/include/oj_control.hpp` | 新增 `GetUserStats(user, result, err_code)` 方法 |
| `src/oj_server/src/oj_server.cpp` | 新增路由 `GET /api/user/stats` |

**前端修改**：
- `user/profile.html` — 调用 `/api/user/stats`，渲染统计数据和最近提交列表

---

### Feature 3: 性能优化

**涉及场景**：题解板块、题解评论、登录/注册

**第一步：代码层面分析（已知瓶颈）**

| 瓶颈位置 | 问题描述 | 优化建议 |
|----------|----------|----------|
| `Model::CreateConnection()` | 每次查询创建新 MySQL 连接，无连接池 | 高优先：实现 MySQL 连接池或改用 mysql-connector-c++ 连接池 |
| `GetCommentsBySolutionId` | 每条评论都 `GetUserById` 做 N+1 查询 | 中优先：改用 LEFT JOIN 一次查出 |
| `GetSolutionList` | 同样 N+1 问题（每条题解查一次作者名） | 中优先：同上，改 JOIN |
| `AllQuestions` | 首次未命中时全量扫描 | 已有 Redis 缓存，观察命中率 |
| HTML 缓存 | ctemplate 渲染每次重新解析模板 | 已有 view 内存缓存和 Redis 缓存 |
| `DeleteComment` | 无连接池下多次查询（查 solution_id → 删除 → 更新计数） | 低优先：合并为事务 |

**第二步：实际运行测试**

- 使用现有 `tools/c2_benchmark_compare.sh`（需 live server）
- 或编写简单 curl/ab 压测脚本
- 测试维度：P50/P95/P99 延迟、QPS
- 目标场景：题解列表页、评论加载、验证码发送、登录/注册

**第三步：优化落地**

- **高优先**：创建 MySQL 连接池 `ConnectionPool` 类，替换所有 `CreateConnection()` 调用
- **中优先**：N+1 查询合并（题解列表、评论列表改 JOIN 查询）
- **低优先**：`DeleteComment` 事务合并

---

## 三、数据库迁移清单

所有 DDL 变更按 OMNI.md 规范放于 `docs/db_migrations/`，命名格式 `YYYYMMDD_HHMM_description.sql`

| 序号 | 文件名 | 变更内容 |
|------|--------|----------|
| 1 | `20260428_0001_add_user_avatar.sql` | `users` 表增加 `avatar_path VARCHAR(255) DEFAULT ''` |
| 2 | `20260428_0002_add_submits_indexes.sql` | `user_submits` 表增加 `(user_id, is_pass)` 联合索引（为统计查询优化） |

---

## 四、新增 API 路由汇总

| 方法 | 路由 | 功能 | 需登录 |
|------|------|------|--------|
| POST | `/api/user/avatar` | 上传头像 | ✅ |
| DELETE | `/api/user/avatar` | 删除头像 | ✅ |
| GET | `/api/question/{id}/sample_tests` | 获取默认测试用例 | ❌ |
| POST | `/api/question/{id}/test` | 临时/自定义测试 | ✅ |
| GET | `/api/question/{id}/submits` | 查看某题提交记录 | ✅ |
| GET | `/api/user/stats` | 用户统计信息 | ✅ |

---

## 五、修改文件一览

| 文件 | 修改类型 | 描述 |
|------|----------|------|
| `src/comm/comm.hpp` | 修改 | User 结构体加 avatar_path 字段 |
| `src/oj_server/include/oj_model.hpp` | 修改 | 新增多个方法（GetSampleTests, GetUserSubmitsByQuestion, GetUserStats, UpdateUserAvatar, 连接池） |
| `src/oj_server/include/oj_control.hpp` | 修改 | 新增业务方法（RunSingleTest, UploadAvatar, GetUserSubmits, GetUserStats）；修复 DeleteComment |
| `src/oj_server/src/oj_server.cpp` | 修改 | 新增路由；修复 DELETE handler try-catch；设置 avatars 静态目录 |
| `src/wwwroot/one_question.html` | 修改 | 添加测试用例/测试结果/提交记录区域；题解详情内嵌切换；点赞/收藏绑定 |
| `src/wwwroot/solutions/detail.html` | 修改 | 确保点赞/收藏/评论完整可用 |
| `src/wwwroot/user/profile.html` | 修改 | 添加头像上传；提交统计展示 |
| `src/wwwroot/js/auth.js` | 修改 | SERVER_USER_INFO 增加 avatar_url |
| `docs/db_migrations/20260428_0001_add_user_avatar.sql` | 新增 | avatar_path 字段 |
| `docs/db_migrations/20260428_0002_add_submits_indexes.sql` | 新增 | user_submits 索引 |

---

## 六、开发优先级与依赖关系

```
Bug #1 评论删除段错误修复 (独立，最高优先)
    │
    ├── Feature 2.3 点赞/收藏 (纯前端，无后端改动)
    │
    ├── Feature 2.2 题解详情内嵌 (纯前端改动)
    │
    ├── Feature 2.1 用户头像 (DB + 后端 + 前端)
    │
    ├── Feature 2.4 提交记录 (后端API + 前端)
    │   └── 依赖: user_submits 表查询
    │
    ├── Feature 2.6 用户统计 (后端API + 前端)
    │   └── 依赖: user_submits 聚合查询
    │
    ├── Feature 2.5 测试用例和测试 (最复杂)
    │   ├── 2.5a 获取默认测试用例 API
    │   └── 2.5b 临时/自定义测试执行
    │
    └── Feature 3 性能优化 (贯穿全程)
        ├── 代码分析 → 修复 N+1
        ├── MySQL 连接池
        └── 实际压测验证
```

---

## 七、风险提示

1. **Bug #1 段错误根因**：需实际编译运行后用 GDB/AddressSanitizer 确认崩溃点，当前分析基于代码审阅
2. **Security**：头像上传需要严格校验文件类型和大小，防止任意文件写入
3. **SQL 注入**：当前 Model 层部分方法拼接 SQL（如 DeleteComment），需确保所有用户输入使用 `EscapeSqlString`
4. **并发**：点赞/收藏的 ToggleSolutionAction 使用 SELECT + INSERT/DELETE 两步操作，存在竞态条件，应使用 INSERT ON DUPLICATE KEY 或事务
5. **compile_server 通信**：临时测试需要重新构建请求 JSON，但不需要改 compile_server 协议
6. **前端文件组织**：部分页面是 SSR（ctemplate），部分是 SPA，还有独立 HTML + inline JS。新增功能需确定放在哪种页面中
7. **docs/db_migrations 目录**：当前不存在，需创建