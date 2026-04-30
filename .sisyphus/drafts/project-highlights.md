# OnlineJudge 项目亮点分析

## 项目规模

- **代码总量**: 14,006 行 C++
- **模块数**: 13 个独立模块（重构后）
- **Redis 缓存操作点**: 30 处
- **近期提交**: 47 个

---

## 一、性能优化

### 1. 多层 Redis 缓存体系

采用 Cache-Aside 模式，构建了 8 层独立缓存：

| 缓存层 | Key 格式 | TTL | 位置 |
|--------|----------|-----|------|
| 用户信息 | `user:id:{uid}` | 3600±600s | `model_user.hpp` |
| 题解列表 | `oj:prod:v2:solution:list:...` | 600±120s | `model_solution.hpp` |
| 题解详情 | `oj:prod:v2:solution:detail:...` | 600±120s | `model_solution.hpp` |
| 评论列表 | `comment:list:sid:{id}:page:...` | 300±60s | `model_comment.hpp` |
| 回复列表 | `reply:list:pid:{id}:page:...` | 120±30s | `model_comment.hpp` |
| 评论操作 | `action:user:{uid}:comment:{cid}` | 300±60s | `model_comment.hpp` |
| 题目详情 | `oj:prod:v2:detail:data:{id}` | 3600±300s | `model_question.hpp` |
| HTML 页面 | `oj:prod:v2:static:html:{page}` | 21600±3600s | `oj_cache.hpp` |

**亮点**: `BuildJitteredTtl(base, jitter)` 为每个缓存键添加随机偏移，防止缓存同时过期导致的「惊群效应」（Thundering Herd）。

### 2. N+1 查询消除

通过 SQL JOIN 优化和批量 GROUP BY，将评论系统的 DB 查询量降低了 93%：

```
优化前: 20 条评论 = 41 次 DB 查询 (1 列表 + 20 作者 + 20 回复计数)
优化后: 20 条评论 = 3 次 DB 查询  (1 列表 JOIN + 1 批量 GROUP BY)
```

关键改动：
- `Comment` 结构体新增 `author_name` 字段，复用 SQL JOIN 结果
- `GetTopLevelComments` 用 `GROUP BY parent_id` 批量获取回复计数
- `GetUserById` 增加 read-through 缓存（`user:id:{uid}`）

### 3. 评论发表预热

`CreateComment` 在发表顶级评论后，如果命中列表缓存，直接将新评论 prepend 到缓存 JSON 中，避免下次请求 cache miss。这是一种写入时主动填充缓存的优化策略。

---

## 二、高可用/高稳定性

### 1. 分布式编译集群

```
oj_server (主服务器, :8080)
    │
    ├── compile_server 1 (:8081)
    ├── compile_server 2 (:8082)
    └── compile_server 3 (:8083)
```

`CentralConsole::SmartChoose()` 根据各节点当前负载（`m->Load()`）选择负载最低的编译服务器，实现智能负载均衡。节点离线时自动标记为 offline 并从负载列表中移除。

### 2. 速率限制

`SendEmailAuthCode` 实现了多层速率限制：
- **邮箱冷却**: 60s 内同一邮箱只能发送一次验证码
- **每日邮箱限制**: 20 次/邮箱/天
- **每日 IP 限制**: 50 次/IP/天
- **Redis 存储**: 冷却键和计数键均存储在 Redis 中，分布式环境下保持一致

### 3. 进程隔离

编译服务器使用 `fork() + setrlimit()` 进行进程隔离：
- CPU 时间限制: `RLIMIT_CPU`
- 内存限制: `RLIMIT_AS`
- 独立文件描述符隔离: `open()` + `dup2()`
- 子进程异常退出映射: `WIFSIGNALED` → `ExitCodeToSatusCode`

### 4. 缓存一致性

- **评论创建**: 同时失效 6 个缓存键（评论列表 3 个 size + 回复列表 3 个 size + 题解列表）
- **评论删除**: 级联失效——先删子评论，再删主评论，最后失效缓存
- **题解操作**: 点赞/收藏后立即失效题解详情缓存
- **头像更新**: `UpdateUserAvatar` 同时失效 `user:id:{uid}` 缓存

---

## 三、系统重构/技术选型

### 1. 模块化拆分

将两个巨型文件拆分为 13 个聚焦模块：

```
重构前:
  oj_model.hpp    (3496 行, 40+ 方法) — 单一 God Class
  oj_control.hpp  (2438 行, 50+ 方法) — 单一 God Class

重构后:
  model/  (6 文件, 最大 979 行) — 继承链: Base→Comment→Solution→User→Question→Model
  control/(7 文件, 最大 909 行) — 继承链: Base→Auth→Comment→Solution→User→Question→Control
```

继承链设计使 `oj_server.cpp` 中的路由注册完全不需要改动——`Control` 空壳类通过继承自动获得所有方法。

### 2. 技术栈

| 层 | 技术 | 用途 |
|----|------|------|
| HTTP | cpp-httplib | REST API |
| 模板 | ctemplate | HTML 渲染 |
| 缓存 | sw::redis++ (Redis) | 多层缓存 |
| 数据库 | MySQL C API | 持久化 |
| JSON | jsoncpp | 序列化 |
| 日志 | Logger (自研) | 分级日志 |
| 邮件 | libcurl + SMTP | 验证码 |
| 编辑器 | Editor.md (前端) | Markdown |
| ACE | ACE Editor (前端) | 代码编辑 |

### 3. 设计模式

- **MVC**: Model (`model/`) → View (`oj_view.hpp`) → Controller (`control/`)
- **责任链**: Compile Server Pipeline (`entry → preprocesser → compiler → runner → judge`)
- **Cache-Aside**: 所有 Redis 操作遵循「查缓存 → miss → 查 DB → 写缓存」模式
- **聚合器模式**: `oj_model.hpp` / `oj_control.hpp` 作为聚合头文件

---

## 四、业务流程优化

### 1. 嵌套评论系统

从零实现了二级嵌套评论系统（类似 GitHub Issues）：
- 一级评论可回复，回复显示在折叠面板中
- @引用用户名自动填入回复框
- 回复的回复自动提升到顶级父评论（保持 2 层清晰结构）
- 点赞/编辑/删除操作覆盖所有层级
- Toast 提示发布/编辑/删除成功或失败

### 2. 自定义测试

用户可输入自定义测试数据，代码 + 输入一同提交到编译服务器：
- 编译服务器通过 `g_is_custom_test` (thread_local flag) 区分自定义测试和正常判题
- 自定义测试跳过判题比较，直接返回 stdout/stderr
- 结果实时展示在页面上

### 3. 搜索体验

- **SPA 导航栏搜索**: 输入关键词 → 自动跳转到 `#questions?q=keyword`
- **非 SPA 页面搜索**: 统一注入 `search.js`，Enter 键 → `/all_questions?q=keyword`
- **服务端搜索**: `BuildQuestionWhereClause` 支持标题/编号/难度组合筛选

### 4. 加载体验

- 动态页面（profile、all_questions）显示「正在加载中...」遮罩
- 数据加载完成后平滑过渡显示内容
- 评论区操作后自动刷新，无需手动刷新页面

---

## 五、技术创新

### 1. 文件系统头像解析

`GetEffectiveAvatarUrl(uid)` 不依赖数据库中可能过期的 `avatar_path`，直接在文件系统中扫描 `pictures/avatars/{uid}.{jpg|png|gif|webp}`，找到即返回。这是去中心化的头像解析方案。

### 2. 防缓存雪崩

`BuildJitteredTtl(base_ttl, jitter)` 为每个缓存键添加 thread_local 随机数生成器的偏移，确保批量缓存不会同时过期。

### 3. 列表版本控制

题目/用户列表使用 `list:{type}:version` 键作为版本号。数据变更时 `BumpListVersion()` INCR 版本号，旧缓存键因版本号不同自动失效，无需逐个删除。

### 4. 编译服务器自定义测试标记

`thread_local bool g_is_custom_test` 在 `fork()` 前设置，子进程自动继承该标记。责任链末端的 `HandlerProgramEnd` 根据标记决定是否跳过正常判题流程。

### 5. 评论预写入缓存

发表评论后不等待下次请求 miss，而是主动读取现有缓存 → prepend 新评论 → 写回缓存。这是在 Cache-Aside 模式上的优化变体（Write-Behind Pre-warming）。
