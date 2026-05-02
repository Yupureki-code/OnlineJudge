# OnlineJudge 项目亮点分析

## 项目规模

- **代码总量**: ~14,000 行 C++（重构后优化约 -800 行冗余）
- **模块数**: 13 个独立模块（Model 6 + Controller 7）
- **Redis 缓存操作点**: 30 处
- **提交数**: 70+ 个

---

## 一、性能优化

### 1. 多层 Redis 缓存体系（8 层）

| 缓存层 | Key 格式 | TTL |
|--------|----------|-----|
| 用户信息 | `user:id:{uid}` | 3600±600s |
| 题解列表 | `oj:prod:v2:solution:list:...` | 600±120s |
| 题解详情 | `oj:prod:v2:solution:detail:...` | 600±120s |
| 评论列表 | `comment:list:sid:{id}:page:...` | 300±60s |
| 回复列表 | `reply:list:pid:{id}:page:...` | 120±30s |
| 评论操作 | `action:user:{uid}:comment:{cid}` | 300±60s |
| 题目详情 | `oj:prod:v2:detail:data:{id}` | 3600±300s |
| HTML 页面 | `oj:prod:v2:static:html:{page}` | 21600±3600s |

**核心创新**: `BuildJitteredTtl(base, jitter)` — 每个缓存键有随机 TTL 偏移，防止惊群效应。

### 2. N+1 查询消除（-93% DB 查询）

| 指标 | 优化前 | 优化后 |
|------|:--:|:--:|
| 20 条评论 | 41 次 | 3 次 (-93%) |
| 10 条回复 | 31 次 | 2 次 (-94%) |

关键改动：SQL JOIN 预取作者名、`GROUP BY parent_id` 批量回复计数、`GetUserById` read-through 缓存。

### 3. 评论发表预热（Write-Behind Pre-warming）

`CreateComment` 成功后主动读取列表缓存 → prepend 新评论 → 写回缓存，避免下次请求 miss。

### 4. 编译错误路径修复

编译器将 g++ stderr 重定向到 `.stderr` 文件，与 `HandlerProgramEnd` 读取路径对齐，编译错误信息正确返回到前端。

---

## 二、高可用 / 高稳定性

### 1. 分布式编译集群 + 智能负载均衡

```
oj_server (:8080) → SmartChoose → compile_server 集群 (:8081/8082/8083)
```

自动离线检测、负载最低优先选择。

### 2. 多层认证速率限制

| 限制 | 值 | 存储 |
|------|:--:|------|
| 邮箱冷却 | 60s | Redis |
| 每日邮箱 | 20 次 | Redis |
| 每日 IP | 50 次 | Redis |
| 验证码尝试 | 5 次 | Redis |

### 3. 进程隔离（双重 fork + setrlimit）

`fork()` → `setrlimit(RLIMIT_CPU, RLIMIT_AS)` → `open()` + `dup2()` → `exec()`，信号异常映射 `WIFSIGNALED → ExitCodeToSatusCode`。

### 4. 级联缓存一致性

评论创建同时失效 6 个缓存键，删除级联失效，操作后立即更新缓存。

### 5. 会话容错

Redis 故障时，Session 自动回退到本地 `std::map` 内存存储。

---

## 三、系统重构 / 技术选型

### 1. 继承链 → 组合重构

```
重构前（继承链 5 层深）:
  ModelBase → Comment → Solution → User → Question → Model
  ControlBase → Auth → Comment → Solution → User → Question → Control

重构后（组合 + 扁平）:
  Model : ModelBase {
      ModelComment  _comment;    → Comment()
      ModelSolution _solution;   → Solution()
      ModelUser     _user;       → User()
      ModelQuestion _question;   → Question()
  }
```

**效果**: 调用方从 `_model.GetCommentsById()` 变为 `_model.Comment().GetCommentsById()`，模块归属一目了然。

### 2. 样板代码消除（净 -800 行）

| 优化 | 消除量 |
|------|:--:|
| `QueryMySql` 辅助方法 | 107 处 mysql_query 模板 |
| `replyJson` Lambda | 64 处 JSON 响应构建 |
| `requireAuth` Lambda | 20 处认证检查 |
| `parseBody` Lambda | 13 处 JSON Body 解析 |
| 21 个 OPTIONS preflight | 1 个通配兜底 |

### 3. 权限分离

管理员路由（增删题目、刷新缓存）从 8080 迁移到 8090（`oj_admin`），使用独立的 admin session 认证。

### 4. 技术栈

| 层 | 技术 |
|----|------|
| HTTP | cpp-httplib |
| 模板 | ctemplate |
| 缓存 | Redis (sw::redis++) |
| 数据库 | MySQL C API (RAII unique_ptr wrapper) |
| JSON | jsoncpp |
| 前端 | ACE Editor / Editor.md |
| 构建 | CMake |

---

## 四、业务流程优化

### 1. 两级嵌套评论系统

- 一级评论可回复，回复展示在折叠面板中
- @用户名自动填入回复框
- 回复的回复自动提升到顶级（保持 2 层清晰结构）
- 点赞/编辑/删除覆盖所有层级
- Toast 提示发布/编辑/删除成功或失败

### 2. 自定义测试

`thread_local g_is_custom_test` 标记 + 自定义输入 → 编译运行 → 跳过判题直接返回 stdout/stderr。

### 3. 全站搜索统一

10 个页面统一注入 `search.js`，Enter 键触发搜索跳转。

### 4. 头像系统去 DB 化

完全移除 `avatar_path` 列，改为文件系统扫描 `pictures/avatars/{uid}.{jpg|png|gif|webp}`。零 DB 依赖，文件即状态。

### 5. 页面加载体验

动态页面显示 loading 遮罩，评论区操作自动刷新，无需手动重载。

---

## 五、技术创新

### 1. 文件系统头像解析（去中心化）

`GetEffectiveAvatarUrl(uid)` 扫描文件系统，不依赖任何数据库字段。

### 2. 防缓存雪崩（Jittered TTL）

`thread_local mt19937` 随机数生成器为每个缓存键添加随机 TTL 偏移。

### 3. 列表版本控制（零删除失效）

`Redis INCR` 版本号，旧缓存键因版本号变化自然失效，无需逐个删除。

### 4. thread_local 跨 fork 标志传递

`g_is_custom_test` 在 fork 前设置，子进程自动继承，责任链末端据此决定是否跳过判题。

### 5. 评论预写入缓存

发表后主动读取现有缓存 → prepend → 写回，即时更新。

### 6. 组合优于继承（API 语义化）

从 "魔法继承链" 改为显式组合访问器，调用方天然知道每个方法属于哪个模块。
