# OnlineJudge

## 版权说明

此项目基于 C++ 分布式架构在线 OJ 平台，仅供学习使用。禁止用于商业目的。

---

## 项目简介

基于 C++17 的分布式在线编程评测系统。用户可以在线编写代码、提交判题、查看题解、参与讨论。采用分布式架构——主服务器处理 HTTP 请求和业务逻辑，编译服务器集群负责代码编译和进程隔离运行。

**核心能力**: 分布式判题、两层嵌套评论、8 层 Redis 缓存、管理后台仪表盘、自定义测试。

---

## 1. 项目架构

### 1.1 技术栈

| 层 | 技术 |
|----|------|
| 语言 | C++17 |
| HTTP | cpp-httplib |
| 模板 | ctemplate |
| JSON | jsoncpp |
| 数据库 | MySQL 8.0 |
| 缓存 | Redis 7.0 (sw::redis++) |
| 加密 | OpenSSL (SHA-256) |
| 邮件 | libcurl + SMTP |
| 前端 | ACE Editor + Editor.md |
| 构建 | CMake |

### 1.2 服务架构

```
                   ┌──────────┐
                   │  Browser │
                   └────┬─────┘
                        │
          ┌─────────────┼─────────────┐
          │                           │
    ┌─────▼─────┐              ┌──────▼──────┐
    │ oj_server │              │   oj_admin  │
    │   :8080   │              │    :8090    │
    └─────┬─────┘              └─────────────┘
          │
   ┌──────┼──────┐
   │      │      │
┌──▼──┐ ┌▼──┐ ┌─▼──┐
│MySQL│ │Redis│ │日志│
└─────┘ └───┘ └────┘
          │
          │ SmartChoose 负载均衡
          │
   ┌──────┼──────┬──────┐
   │      │      │      │
┌──▼──┐ ┌▼──┐ ┌─▼──┐ ┌─▼──┐
│:8081│ │8082│ │8083│ │8084│  compile_server 集群
└─────┘ └───┘ └────┘ └────┘
```

### 1.3 模块架构

```
src/oj_server/include/
├── model/                     Model 层 (6 文件)
│   ├── model_base.hpp         MySQL 连接 + 缓存工具 + 指标记录
│   ├── model_comment.hpp      评论 CRUD
│   ├── model_solution.hpp     题解 CRUD + 缓存
│   ├── model_user.hpp         用户 CRUD + 统计
│   ├── model_question.hpp     题目 CRUD + 测试用例
│   └── oj_model.hpp           聚合 + 管理员 + 指标刷新线程
│
├── control/                   Controller 层 (7 文件)
│   ├── control_base.hpp       判题调度 + 负载均衡 + 头像解析
│   ├── control_auth.hpp       认证 + 密码哈希 + 速率限制
│   ├── control_comment.hpp    评论 CRUD + 批量计数
│   ├── control_solution.hpp   题解 CRUD + 互动
│   ├── control_user.hpp       头像 + 统计
│   ├── control_question.hpp   题目 + 自定义测试
│   └── oj_control.hpp         聚合 (15 行)
│
├── oj_cache.hpp               Redis 缓存层 (Jittered TTL + 版本控制)
├── oj_view.hpp                HTML 模板渲染
├── oj_session.hpp             会话管理 (Redis + 本地回退)
├── oj_mail.hpp                SMTP 邮件
└── oj_admin.hpp               管理后台 (会话 + 审计)

src/compile_server/include/
├── COP_hanlder.hpp            责任链基类 + 状态枚举
├── entry.hpp                  流水线入口
├── preprocesser.hpp           源文件生成
├── compiler.hpp               g++ 编译
├── runner.hpp                 fork + setrlimit 进程隔离
└── judge.hpp                  输出比对
```

### 1.4 设计模式

| 模式 | 应用 |
|------|------|
| MVC | Model (`model/`) → View (`oj_view.hpp`) → Controller (`control/`) |
| 责任链 | compile_server: preprocesser → compiler → runner → judge |
| Cache-Aside | Redis miss → MySQL → 写回 Redis |
| 组合优于继承 | Model/Controller 扁平组合 + 访问器 (`_model.Comment().xxx()`) |

---

## 2. 核心模块

### 2.1 判题系统

用户提交代码 → oj_server 选择负载最低的编译服务器 → 编译 → 运行 → 判题 → 返回结果。

**进程隔离**: `fork()` + `setrlimit(RLIMIT_CPU)` + `setrlimit(RLIMIT_AS)` + `dup2()` + `exec()`

**判题状态**: AC / WA / RTE / MLE / CE / FPE / SEGV / TLE

**自定义测试**: `thread_local bool g_is_custom_test` 标记，编译运行用户输入的测试数据，跳过正常判题直接返回 stdout/stderr。

### 2.2 缓存系统

**8 层 Redis 缓存**:

| 缓存层 | TTL |
|--------|-----|
| 用户信息 `user:id:{uid}` | 3600±600s |
| 题解列表/详情 | 600±120s |
| 评论列表 | 300±60s |
| 回复列表 | 120±30s |
| 评论操作 | 300±60s |
| 题目详情 | 3600±300s |
| HTML 页面 | 21600±3600s |

**防雪崩**: `BuildJitteredTtl(base, jitter)` 添加随机偏移。

**版本控制**: `Redis INCR` 版本号，旧缓存自动失效，零删除。

**缓存指标**: 按业务类型（Question/Auth/Comment/Solution/User）记录每次 DB 操作延迟和命中。后台线程 30s/100 条触发写入 MySQL，管理后台仪表盘展示命中率趋势。

**N+1 消除**: `GROUP BY parent_id` 批量获取回复计数（-93% MySQL 查询）。

### 2.3 评论系统

- 两级嵌套：一级评论 + 二级回复
- @引用：自动填入 `@用户名`，`reply_to_user_id` 存储
- 折叠展开：「▼ N 条回复」
- 点赞/编辑/删除 + Toast 提示

### 2.4 认证系统

**邮箱验证码**: 发送 → 验证 → 自动登录/注册

**速率限制**: 60s 冷却 + 20 次/邮箱/天 + 50 次/IP/天 + 5 次验证码尝试

**密码哈希**: `sha256_iter_v1` — `salt:password:pepper` × 120000 迭代 SHA-256

### 2.5 管理后台

访问 `:8090/admin`，管理员登录后可使用：

- **仪表盘**: 统计 + 缓存命中率面板（5 业务类型）
- **题目管理**: CRUD + 测试用例编辑（滚动表格 + 内联编辑）
- **管理员账户**: CRUD
- **缓存刷新**: 版本 INCR

---

## 3. 部署

### 3.1 编译

```bash
mkdir build && cd build
cmake .. -DHOST=localhost -DUSER=oj_server -DPASSWD=password \
         -DMYOJ=myoj -DPORT=3306 -DREDIS=tcp://127.0.0.1:6379
make -j$(nproc)
```

### 3.2 数据库

```bash
mysql -u root -p myoj < sql/base_schema.sql
mysql -u root -p myoj < sql/migrations/20260428_01_nested_comments.sql
mysql -u root -p myoj < sql/migrations/20260502_01_cache_metrics.sql
```

### 3.3 启动

```bash
./output/compile_server 8081 &   # 编译服务器（可多实例）
./output/oj_server &             # 主服务器 :8080
./output/oj_admin &              # 管理后台 :8090
```

### 3.4 环境变量

| 变量 | 用途 |
|------|------|
| `OJ_REDIS_ADDR` | Redis 地址 |
| `OJ_SMTP_HOST/USER/PASS/PORT/FROM` | SMTP 邮件 |
| `OJ_PASSWORD_PEPPER` | 密码哈希额外盐 |

---

## 4. API 参考

### 认证
`POST /api/auth/send_code` | `POST /api/auth/verify_code` | `POST /api/user/password/login` | `POST /api/user/logout`

### 题目
`GET /api/questions` | `GET /api/question/{id}` | `POST /api/question/{id}/test` | `GET /api/question/{id}/pass_status`

### 题解
`GET/POST /api/questions/{id}/solutions` | `GET /api/solutions/{id}` | `POST /api/solutions/{id}/like`

### 评论
`GET/POST /api/solutions/{id}/comments` | `PUT/DELETE /api/comments/{id}` | `GET /api/comments/{id}/replies` | `POST /api/comments/{id}/like`

### 用户
`GET /api/user/info` | `GET /api/user/stats` | `POST/DELETE /api/user/avatar`

### 管理 (:8090)
`POST /api/admin/auth/login` | `GET /api/admin/overview` | `GET /api/admin/cache/metrics` | `GET/POST/PUT/DELETE /api/admin/questions/*` | `GET/POST/PUT/DELETE /api/admin/tests/*`

---

## 5. 项目亮点

### 性能
- 8 层 Redis 缓存 + Jittered TTL 防雪崩
- N+1 查询消除 (-93%)：JOIN + GROUP BY 批量化
- 样板消除 (-800 行)：`QueryMySql` / `replyJson` / `requireAuth` / `parseBody`
- 评论预写入缓存

### 架构
- Model 3496 行 → 6 模块 / Controller 2438 行 → 7 模块
- 继承链 5 层 → 扁平组合 + 语义化访问器
- 107 处 MySQL 模板 → 统一 `QueryMySql()`

### 稳定性
- 分布式编译集群 + 智能负载均衡 + 自动故障下线
- Redis 会话 + 本地内存回退
- 三层速率限制
- RAII MySQL 连接

### 创新
- 文件系统头像解析（零 DB 依赖）
- `thread_local` 跨 fork 标记
- 列表版本控制（Redis INCR 零删除失效）
- Cache Metrics V2：按类型 + MySQL 持久化 + 混合刷新
