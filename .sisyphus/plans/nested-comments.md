# 嵌套评论实现计划

## TL;DR

> **目标**: 为 OJ 平台题解评论区实现多级嵌套评论，支持回复一级评论、对嵌套评论点赞、引用嵌套评论回复。
> **关键发现**: 后端（model/controller/route）已完整支持嵌套评论，`solutions/detail.html` 已有基础回复 UI。主要工作是修复迁移文件、增强 `one_question.html` 的回复 UI、让嵌套评论支持点赞和回复。
> **预估工时**: 中等（5 个后端修复 + 8 个前端增强）
> **并行执行**: YES — Wave 1 后端迁移 + Wave 2 前端双页面并行

---

## Context

### 当前状态
- **DB schema**: `solution_comments` 表缺少 `parent_id` 列（迁移文件有 bug）
- **Model**: 完整实现了嵌套评论的 CRUD、点赞/收藏、级联删除
- **Controller**: `PostComment(parent_id=...)` 已支持嵌套、`GetCommentReplies(parent_id)` 获取回复
- **Routes**: 8 个评论相关端点全部注册
- **`solutions/detail.html`**: 已有回复按钮、回复表单、回复折叠展开、点赞/收藏按钮，但回复渲染为只读
- **`one_question.html`**: 完全没有回复功能，只有平铺的一级评论

### 数据结构
```
Comment {
  parent_id = 0  → 一级评论
  parent_id = X  → X 的回复（直接子级）
  reply_to_user_id → @引用的用户 ID
}
```
当前仅支持 **单级嵌套**（`GetCommentReplies` 只查直接子级）。

---

## Work Objectives

### 核心目标
为两个题解评论区页面实现完整的多级嵌套评论功能。

### 具体交付物
- [ ] 修复迁移文件：添加 `parent_id` 列
- [ ] `one_question.html` 增加回复功能（按钮/表单/加载/渲染）
- [ ] `solutions/detail.html` 回复支持点赞、编辑、删除
- [ ] 两个页面支持回复嵌套评论（回复回复）
- [ ] `solutions/detail.html` 回复增加"回复"按钮以支持多层嵌套

### 定义完成
- 在 `one_question.html` 可对一级评论点击"回复"并提交
- 回复提交后页面刷新显示新回复
- 可展开/折叠回复列表
- 对任意评论（含回复）可点赞
- 对回复可再次回复（多层嵌套）

---

## Verification Strategy

### 测试决策
- **已存在基础设施**: 否（项目无测试框架）
- **自动化测试**: 否
- **Agent QA**: 每个 TODO 附带 Playwright 端到端验证场景

### QA 策略
所有验证通过 Playwright 自动化：提交评论 → 提交回复 → 验证回复显示 → 展开/折叠回复 → 对回复点赞 → 回复嵌套评论

---

## Execution Strategy

### 并行执行波次

```
Wave 1 (后端基础 - 立即开始):
├── Task 1: 修复迁移文件 (添加 parent_id 列)
├── Task 2: 验证后端嵌套评论 API 正常工作

Wave 2 (前端双页面并行):
├── Task 3: one_question.html - 添加回复按钮 + 回复表单
├── Task 4: one_question.html - 加载并渲染回复列表
├── Task 5: one_question.html - 回复提交逻辑 + parent_id 传递
├── Task 6: solutions/detail.html - 回复渲染增强（点赞/编辑/删除）
├── Task 7: solutions/detail.html - 回复增加"回复"按钮（多层嵌套）

Wave 3 (样式和整合):
├── Task 8: CSS 样式统一（回复缩进、层级标识）
├── Task 9: 端到端 Playwright 验证

Critical Path: Task 1 → Task 2 → Task 3/6 → Task 4/5/7 → Task 8/9
```

---

## TODOs

- [ ] 1. 修复迁移文件：添加 parent_id 列

  **What to do**:
  - 在 `sql/migrations/20260428_01_nested_comments.sql` 的 ALTER TABLE 语句开头添加 `ADD COLUMN parent_id INT UNSIGNED DEFAULT NULL COMMENT '父评论ID，NULL=顶级评论' AFTER content,`
  - 验证迁移语法正确（`AFTER content` 而非 `AFTER id`，因为数据库中已有部分列）
  - 若需要，同步更新 `base_schema.sql` 中的 `solution_comments` 表定义

  **Must NOT do**:
  - 不要删除已有的 `AFTER parent_id` 引用——改为先添加列再引用
  - 不要修改已有的 `comment_actions` 表定义

  **QA Scenarios**:
  ```
  Scenario: 迁移文件语法验证
    Tool: Bash
    Steps:
      1. 读取迁移文件确认 parent_id 在 reply_to_user_id 之前添加
      2. grep "ADD COLUMN.*parent_id" sql/migrations/20260428_01_nested_comments.sql
    Expected Result: 找到 parent_id ADD COLUMN 语句
    Evidence: .sisyphus/evidence/task-1-migration.txt
  ```

  **Commit**: YES
  - Message: `fix(migration): add missing parent_id column for nested comments`
  - Files: `sql/migrations/20260428_01_nested_comments.sql`

---

- [ ] 2. 验证后端嵌套评论 API

  **What to do**:
  - 确认 `GET /api/comments/{id}/replies` 端点在 `oj_server.cpp:1811` 已注册且工作正常
  - 确认 `POST /api/solutions/{id}/comments` 接受 `parent_id` 参数（oj_control.hpp:1144）
  - 确认 `POST /api/comments/{id}/like` 端点 (oj_server.cpp:1932) 已注册
  - 确认 `POST /api/comments/{id}/favorite` 端点 (oj_server.cpp:1965) 已注册
  - 阅读 oj_control.hpp 第 1099-1205 行确认 PostComment 支持嵌套逻辑
  - 阅读 oj_model.hpp 第 1798-1874 行确认 GetCommentReplies 查询正确

  **Must NOT do**:
  - 不要修改后端逻辑（已验证完整）
  - 不要添加新端点

  **QA Scenarios**:
  ```
  Scenario: curl 测试嵌套评论 API
    Tool: Bash (curl)
    Preconditions: 服务运行在 localhost:8080，已有 solution #1，用户已登录获取 session
    Steps:
      1. POST /api/solutions/1/comments -d '{"content":"顶级评论","parent_id":0}' → 获取 comment_id
      2. POST /api/solutions/1/comments -d '{"content":"回复测试","parent_id":<comment_id>}' → 验证返回 success:true
      3. GET /api/comments/<comment_id>/replies → 验证返回该回复
    Expected Result: 创建回复成功，API 返回包含 parent_id 的评论数据
    Evidence: .sisyphus/evidence/task-2-api.txt
  ```

  **Commit**: YES | NO (视情况，可能无需变更)
  - Message: `docs: verify nested comment API endpoints`
  - Files: 无（仅验证）

---

- [ ] 3. `one_question.html` — 添加回复按钮和回复表单

  **What to do**:
  **参考**: `solutions/detail.html` 第 805-865 行的完整回复实现
  - 在 `renderDetailComment` 函数（line 2448）中添加"回复"按钮（在编辑/删除按钮之前）
  - 类似 detail.html 第 806-808 行的模式：`replyBtn.onclick = function() { showReplyForm(comment.id, comment.author_name); }`
  - 在评论项下方添加隐藏的回复表单 (id=`reply-form-{commentId}`)，包含 textarea + 回复/取消按钮
  - 添加 `showReplyForm(parentId, username)` 和 `hideReplyForm(parentId)` 函数
  - 回复表单用 `display:none` 默认隐藏，点击"回复"按钮切换显示
  - textarea 预填充 `@用户名 ` 前缀

  **Must NOT do**:
  - 不要破坏现有的编辑/删除功能
  - 不要修改提交一级评论的逻辑（submitDetailComment）

  **QA Scenarios**:
  ```
  Scenario: 点击回复按钮显示表单
    Tool: Playwright
    Steps:
      1. 打开题目详情页 → 切换到题解 Tab → 点击任意题解 → 滚动到评论区
      2. 在一级评论上点击"回复"按钮
      3. 验证回复表单出现（textarea + 回复/取消按钮）
      4. 验证 textarea 已预填 @用户名
      5. 点击"取消"按钮验证表单隐藏
    Expected Result: 回复表单正确显示/隐藏，textarea 包含 @username 前缀
    Evidence: .sisyphus/evidence/task-3-reply-form.png
  ```

  **Commit**: YES
  - Message: `feat(one_question): add reply button and reply form to comments`
  - Files: `src/wwwroot/one_question.html`

---

- [ ] 4. `one_question.html` — 加载并渲染回复列表

  **What to do**:
  **参考**: `solutions/detail.html` 第 836-856 行（回复折叠/展开）和第 898-911 行（loadReplies）
  - 在 `renderDetailComment` 中，顶级评论底部添加"▼ N 条回复"折叠按钮
  - 创建隐藏的 reply-container div 用于存放回复
  - 点击折叠按钮时调用 `loadReplies(commentId, container)`
  - `loadReplies` 调用 GET `/api/comments/{id}/replies?page=1&size=50`
  - 实现 `renderCommentInline(comment, container)` 函数渲染回复
  - 回复渲染包含：头像、作者名（@reply_to_user_name 前缀）、时间、内容
  - 首次点击后设置 `container.dataset.loaded = 'true'` 防止重复加载

  **Must NOT do**:
  - 不要一次性展开所有回复（按需加载）
  - 不要修改现有的 renderDetailComment 函数名（避免 break 其他引用）

  **QA Scenarios**:
  ```
  Scenario: 展开回复列表
    Tool: Playwright
    Steps:
      1. 进入有回复的题解评论区
      2. 点击 "▼ N 条回复" 按钮
      3. 验证回复列表展开，显示每条回复的头像、作者、内容
      4. 验证按钮文本变为 "▲ N 条回复"
      5. 再次点击验证回复列表折叠
    Expected Result: 回复正确展开/折叠
    Evidence: .sisyphus/evidence/task-4-replies.png

  Scenario: 空回复
    Tool: Playwright
    Preconditions: 一级评论没有回复
    Steps:
      1. 查看没有回复的评论
      2. 验证不显示 "▼ 0 条回复" 按钮
    Expected Result: reply_count=0 时不显示折叠按钮
    Evidence: .sisyphus/evidence/task-4-empty.png
  ```

  **Commit**: YES
  - Message: `feat(one_question): add reply loading and rendering for nested comments`
  - Files: `src/wwwroot/one_question.html`

---

- [ ] 5. `one_question.html` — 回复提交逻辑

  **What to do**:
  **参考**: `solutions/detail.html` 第 944-965 行（submitReply）
  - 实现 `submitReply(parentId)` 函数
  - 获取 textarea 内容，验证非空
  - POST `/api/solutions/{solutionId}/comments` 携带 `{content, parent_id}`
  - 成功后刷新评论区（调用 loadDetailComments 从第 1 页重新加载）
  - 实现 `window.submitReply = submitReply` 暴露到全局（内联 onclick 需要）
  - 实现 `window.showReplyForm = showReplyForm`
  - 实现 `window.hideReplyForm = hideReplyForm`

  **Must NOT do**:
  - 不要修改 submitDetailComment 的逻辑（一级评论提交保持不变）
  - 不要忘记暴露全局函数（内联 onclick 需要 window.xxx）

  **QA Scenarios**:
  ```
  Scenario: 提交回复
    Tool: Playwright
    Steps:
      1. 打开回复表单 → 输入 "这是回复内容" → 点击"回复"
      2. 验证表单隐藏
      3. 验证评论区刷新，显示刚提交的回复
      4. 验证 @用户名 正确显示
      5. 验证 reply_count 更新
    Expected Result: 回复提交成功，评论区正确刷新
    Evidence: .sisyphus/evidence/task-5-submit-reply.png
  ```

  **Commit**: YES
  - Message: `feat(one_question): implement reply submission with parent_id`
  - Files: `src/wwwroot/one_question.html`

---

- [ ] 6. `solutions/detail.html` — 回复增强：点赞、编辑、删除

  **What to do**:
  - 重构 `renderCommentInline` 函数（line 914-926），改为完整渲染（类同 `renderComment`）
  - 为每条回复添加：❤ 点赞按钮、⭐ 收藏按钮、编辑按钮（仅自己）、删除按钮（自己或管理员）
  - 点赞逻辑复用 `toggleCommentLike(commentId, btn)`（line 872）
  - 收藏逻辑复用 `toggleCommentFavorite(commentId, btn)`（line 885）
  - 编辑逻辑复用 `editComment(commentId, content)`（line 972）
  - 删除逻辑复用 `deleteComment(commentId)`（line 1000）
  - 回复作者名保留 `@reply_to_user_name` 前缀格式

  **Must NOT do**:
  - 不要修改 renderComment 的逻辑（一级评论保持不变）
  - 不要修改 API 调用（已有端点）

  **QA Scenarios**:
  ```
  Scenario: 对回复点赞
    Tool: Playwright
    Steps:
      1. 展开任意评论的回复列表
      2. 点击某条回复的 ❤ 按钮
      3. 验证按钮计数 +1，颜色变为红色
      4. 再次点击验证取消点赞，技术 -1，颜色恢复
    Expected Result: 回复点赞/取消正常工作
    Evidence: .sisyphus/evidence/task-6-like.png

  Scenario: 编辑自己的回复
    Tool: Playwright
    Preconditions: 当前用户是回复的作者
    Steps:
      1. 点击某条回复的"编辑"按钮
      2. 在 prompt 中输入新内容
      3. 验证回复内容更新，显示"(已编辑)"
    Expected Result: 回复内容正确更新
    Evidence: .sisyphus/evidence/task-6-edit.png
  ```

  **Commit**: YES
  - Message: `feat(solutions): add like/edit/delete buttons to comment replies`
  - Files: `src/wwwroot/solutions/detail.html`

---

- [ ] 7. `solutions/detail.html` — 回复上增加"回复"按钮（多层嵌套）

  **What to do**:
  - 在 `renderCommentInline` 渲染的每条回复中添加"回复"按钮
  - 点击"回复"时调用 `showReplyForm(comment.id, comment.author_name)`
  - 将回复表单移到对应的回复评论下方（或复用顶级评论的表单，需传递正确的 parent_id）
  - 注意：当前回复表单 `reply-form-{id}` 是按顶级评论 ID 命名的。对于嵌套回复，需要创建新的表单实例或使用不同的命名策略
  - 建议：在每条回复下方内嵌一个独立的回复表单，格式为 `reply-form-{commentId}`
  - 提交回复时 `parent_id` 设置为被回复评论的 ID

  **Must NOT do**:
  - 不要破坏现有的顶级评论回复表单
  - 避免过多层级嵌套（最多 3-5 层视觉效果，但数据结构无限支持）

  **QA Scenarios**:
  ```
  Scenario: 回复嵌套评论（回复回复）
    Tool: Playwright
    Steps:
      1. 在回复列表中，对某条回复点击"回复"
      2. 验证回复表单出现在该回复下方
      3. 输入内容 → 提交
      4. 验证新回复出现在回复列表中
      5. 验证新回复的 parent_id 指向被回复的评论
      6. 展开新回复的回复列表 → 验证可再次回复
    Expected Result: 多层嵌套回复正常工作
    Evidence: .sisyphus/evidence/task-7-nested-reply.png
  ```

  **Commit**: YES
  - Message: `feat(solutions): support reply-to-reply for multi-level nested comments`
  - Files: `src/wwwroot/solutions/detail.html`

---

- [ ] 8. CSS 样式统一

  **What to do**:
  - `one_question.html`: 添加 `.sol-comment-reply` 样式（左边框缩进标识层级）
  - `one_question.html`: 添加 `.reply-toggle` 样式（折叠按钮）
  - `one_question.html`: 添加 `.reply-form` 样式（回复表单）
  - `solutions/detail.html`: 为嵌套回复（回复的回复）增加更深的缩进
  - 两页面统一：回复缩进 20px/层，左边框颜色渐变标识层级
  - 确保暗色主题下文字可读

  **Must NOT do**:
  - 不要修改现有的一级评论样式
  - 不要引入新的 CSS 文件（内联 style 标签）

  **QA Scenarios**:
  ```
  Scenario: 回复层级样式
    Tool: Playwright
    Steps:
      1. 创建多层级回复：一级评论 → 回复 A → 回复 B（回复 A 的回复）
      2. 验证每层有 20px 左边距缩进
      3. 验证左边框颜色正确
      4. 验证按钮和文字大小合适
    Expected Result: 层级关系清晰可辨
    Evidence: .sisyphus/evidence/task-8-css.png
  ```

  **Commit**: YES
  - Message: `style: add reply nesting indentation and styling for both pages`
  - Files: `src/wwwroot/one_question.html`, `src/wwwroot/solutions/detail.html`

---

- [ ] 9. 端到端 Playwright 验证

  **What to do**:
  - 编写单一 Playwright 脚本覆盖完整嵌套评论流程
  - 场景：登录 → 打开题解 → 发表一级评论 → 回复一级评论 → 验证回复显示 → 对回复点赞 → 回复回复 → 展开/折叠 → 编辑回复 → 删除回复
  - 在两个页面（`one_question.html` 和 `solutions/detail.html`）分别执行
  - 截图保存到 `.sisyphus/evidence/`

  **QA Scenarios**:
  ```
  Scenario: 完整嵌套评论流程
    Tool: Playwright
    Steps:
      1. 打开题解详情页
      2. 提交顶级评论 "测试一级评论"
      3. 点击"回复" → 输入 "回复一级评论" → 提交
      4. 展开回复列表 → 验证显示 "回复一级评论"
      5. 对回复点击 ❤ → 验证 like_count=1
      6. 对回复点击"回复" → 输入 "回复回复" → 提交
      7. 验证三层嵌套正确显示
      8. 编辑回复内容 → 验证更新
      9. 删除回复 → 验证消失
    Expected Result: 所有操作成功，UI 正确更新
    Evidence: .sisyphus/evidence/task-9-e2e.png
  ```

  **Commit**: YES | NO (仅验证，可能无需提交)

---

## Final Verification Wave

- [ ] F1. **Plan Compliance Audit** — `oracle`
  验证所有 "Must Have" 已实现，所有 "Must NOT Have" 未出现

- [ ] F2. **Code Quality Review** — `unspecified-high`
  检查 JS 语法错误、console.log 残留、内联 onclick 全局函数是否正确暴露

- [ ] F3. **Real Manual QA** — `unspecified-high` + `playwright`
  执行完整的端到端 Playwright 流程，验证两个页面的嵌套评论功能

- [ ] F4. **Scope Fidelity Check** — `deep`
  逐 TODO 核对：迁移文件修复、one_question 回复 UI、detail 回复增强

---

## Commit Strategy

- **Task 1**: `fix(migration): add missing parent_id column for nested comments` — sql/migrations/*.sql
- **Task 3-5**: `feat(one_question): nested comment reply support` — src/wwwroot/one_question.html
- **Task 6-7**: `feat(solutions): full reply features and reply-to-reply` — src/wwwroot/solutions/detail.html
- **Task 8**: `style: reply nesting indentation` — src/wwwroot/one_question.html, src/wwwroot/solutions/detail.html

---

## Success Criteria

### 验证命令
```bash
# 构建
cd build && make -j4

# 启动服务后手动验证或运行 Playwright
```

### 最终检查清单
- [ ] 对一级评论可回复
- [ ] 对回复可再次回复（多层嵌套）
- [ ] 所有评论（含回复）可点赞
- [ ] 回复正确显示 @username 引用
- [ ] 两个页面（one_question + solutions/detail）都支持
- [ ] 迁移文件无 bug
- [ ] 回复折叠/展开正常
