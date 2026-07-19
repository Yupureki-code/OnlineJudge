# OnlineJudge

基于 C++20、OpenResty、Protobuf/brpc、RabbitMQ 和 Docker sandbox 的高性能、高并发开源在线编程评测系统。项目包含用户认证、题目与题解、评论互动、全链路异步判题、自定义测试和独立管理后台，面向学习、部署验证和二次开发。

> 本项目仅供学习和技术研究使用，禁止用于商业目的。

这是一套可实际部署、具备完整业务闭环和工程治理能力的 OJ：浏览器只接触 HTTPS JSON API，内部使用 Protobuf/brpc；提交通过 transactional outbox 可靠进入 RabbitMQ；Judge 使用 inbox 幂等、重试/DLX 和一次性 Docker runner；缓存、管理审计、迁移、过载保护、性能测试与运维入口均包含在仓库中。

## 快速导航

- [项目亮点](#项目亮点)
- [高性能与异步链路](#高性能高并发与端到端异步解耦)
- [功能全景](#功能全景)
- [系统架构](#架构)
- [快速部署](#快速部署)
- [性能基线](#性能基线)
- [完整 API](API.md)

## 项目亮点

### 高性能、高并发与端到端异步解耦

项目的并发模型不是简单地为每个 HTTP 请求创建一个线程，也不是让请求线程同步等待编译和运行结束。请求接入、业务执行、消息投递、Judge 调度、Docker I/O 和结果回调被拆分到不同的执行层，每一层都有独立的并发控制和故障边界。

```text
OpenResty event loop
    |
    | non-blocking upstream I/O + keepalive
    v
brpc I/O / bthread runtime
    |
    | AsyncDispatch
    v
bounded BusinessExecutor thread pool
    |
    | commit submission + outbox, then finish HTTP request
    v
OutboxPublisher background thread
    |
    | AMQP-CPP + libevent + publisher confirm
    v
RabbitMQ durable queue
    |
    | consumer callback + bounded prefetch
    v
C++20 Judge coroutine owner pool
    |
    | co_await DockerTaskAwaitable
    | coroutine suspended, owner thread released
    v
dedicated Docker I/O worker pool
    |
    | completion pushes coroutine back to ready queue
    v
coroutine resume -> signed callback -> inbox complete -> MQ ack
```

#### 接入层：事件驱动而不是一连接一线程

- OpenResty 使用 Nginx event loop 处理大量 keepalive 连接，不为每个浏览器连接长期占用 OS thread。
- `worker_processes auto`、`worker_connections 16384`、`reuseport`、backlog 4096 和 nofile 65536 提供明确的连接容量基础。
- Gateway 到 brpc backend 使用 lua-resty-http cosocket 和 256 连接 keepalive pool，等待 upstream 时不会阻塞整个 Nginx worker。
- JSON 解码、路由匹配、CSRF、字段绑定和 Protobuf 编解码都在统一 Gateway 内完成，浏览器不需要额外协议转换服务。

#### RPC 层：I/O 与业务阻塞隔离

- brpc 负责内部 Protobuf RPC、连接复用和 I/O 调度。
- `AsyncDispatch` 不直接在 brpc 回调中执行数据库业务，而是把任务交给独立 `BusinessExecutor`。
- completion 对象保证 `done` 只执行一次，并统一处理正常完成、取消、线程池停止、队列满和未捕获异常。
- 客户端取消会在业务执行前被识别，避免继续消耗后端资源。
- oj_server 当前配置 16 个 brpc threads、8 个业务 worker 和 256 容量的有界队列；管理服务使用独立的 4/2 线程配置，互不抢占业务线程池。

#### 线程池：有界并发、快速背压、可观测调优

```text
Submit(request):
    if executor stopped:
        return 503 retryable
    if bounded queue full:
        increment rejected_queue_full
        return 503 retryable

    enqueue(request, enqueued_at)
    worker_condition.notify_one()

WorkerLoop():
    task = dequeue()
    observe(queue_wait)
    active_workers += 1
    execute(task)
    observe(execution_time)
    active_workers -= 1
```

- 队列容量是保护数据库和 Redis 的 admission boundary，不会因瞬时流量无限创建线程或无限堆内存。
- 支持 Drain 和 CancelPending 两种停止策略，服务退出时可以选择处理完队列或取消未开始任务。
- accepted、completed、active、queue depth、queue wait、execution time、queue-full reject 均有累计和 max 指标。
- 线程池参数来自实际 4/8/16 worker、128/256/512 queue 对照测试；当前 8/256 是吞吐与共享资源竞争之间的实测折中。

#### 提交链路：HTTP 异步等待 Judge

普通同步 OJ 常把“写数据库、发送消息、编译、运行、判题”放在同一 HTTP 生命周期中，慢任务会长期占用请求线程。这里的提交链路在持久化后立即解耦：

1. BusinessExecutor worker 在一个 MySQL transaction 中写入 submission/custom task 和 outbox。
2. transaction 成功后 HTTP 请求即可返回 `QUEUED`，不等待 Docker 编译和运行。
3. 独立 OutboxPublisher worker 批量 claim 待发送记录，并异步等待 publisher confirm。
4. confirm 成功才把 outbox 标记为 published；超时或 nack 使用指数退避重新投递。
5. RabbitMQ durable queue 吸收短时流量波动，并允许多个独立 Judge 节点竞争消费。

这使 HTTP 接入容量与 Judge 完成容量可以独立扩展，也让 MQ 暂时不可用时已提交任务仍保留在数据库中。

#### Judge：协程调度与阻塞 Docker I/O 分离

`judge_service` 使用自定义 C++20 coroutine event loop，而不是为每个 submission 固定占用一个完整生命周期线程：

- owner worker 只负责创建、恢复和销毁 coroutine，以及推进 Ready/Running/Waiting 状态机。
- 遇到 Docker create/start/exec/read/write/destroy 时，`co_await DockerTaskAwaitable` 将工作提交给专用 I/O worker。
- coroutine 进入 Waiting 后释放 owner worker；同一 owner thread 可以继续推进其他 Judge 任务。
- Docker I/O 完成后，结果和 generation 一起写回 pending state，任务重新进入 ready queue 并恢复执行。
- 每次 I/O 和整个 Judge task 都有 deadline；停止服务时停止接受新 I/O、取消 pending coroutine，并由 owner thread 统一销毁 coroutine handle。
- RabbitMQ prefetch 限制最大在途消息数，避免消费速度超过本机 sandbox 处理能力。

当前每个 Judge 实例使用 4 个 owner workers + 4 个 Docker I/O workers + prefetch 4。参数可调，但压力测试证明在同一 4C Docker 主机上提高到 8/8 只会增加容器竞争，因此项目保留可调能力，同时拒绝把“线程更多”包装成性能提升。

#### 其他异步后台链路

- Admin audit 使用独立有界 worker queue，管理请求不等待审计日志落库。
- Cache metrics 聚合后批量持久化，不在每个命中请求中同步写 MySQL。
- spdlog 使用异步 logger 和 flush tracking，正常请求线程不直接承担逐条磁盘 flush。
- 前端 submission/custom-test poller 使用异步轮询、退避和可恢复 task ID，不阻塞页面主交互。
- RabbitMQ client 基于 AMQP-CPP + libevent，支持异步连接、自动重连、publisher confirm、consumer callback 和 retry/DLX 声明。

#### 并发控制一览

| 层级 | 并发机制 | 当前基线 | 保护手段 |
|---|---|---:|---|
| 浏览器连接 | OpenResty event loop | 16,384 connections/worker | FD 65,536、backlog 4096 |
| Gateway upstream | cosocket + keepalive pool | pool 256 | connect/read/total timeout |
| oj_server RPC | brpc threads | 16、max concurrency 1024 | brpc admission limit |
| 用户业务 | BusinessExecutor | 8 workers、queue 256 | queue full 快速 503 |
| Admin RPC | brpc + 独立 executor | 4 RPC / 2 workers / queue 32 | 与用户业务隔离 |
| Outbox | 后台 worker + confirm callbacks | batch 32 | lease、confirm timeout、retry delay |
| RabbitMQ | durable queue + prefetch | prefetch 4 | retry queues、DLX、manual ack |
| Judge coroutine | owner + I/O 双线程池 | 4 + 4 | deadline、cancel、generation |
| Sandbox | Docker cgroup/namespace | 受 prefetch 约束 | CPU、内存、网络、只读根、超时 |
| 日志/延迟 | async logger + sampled batch | 有界 queue/file | flush tracking、rotation limit |

#### 对“全异步”的准确说明

这里的“全异步链路”指提交从 HTTP 到 MQ、Judge coroutine、Docker I/O、结果回调和 ack 的主流程采用异步边界，不会让 HTTP 请求线程同步等待判题完成。现有 MySQL/Redis Model 中仍有同步调用，它们被隔离在有界 BusinessExecutor 或专用后台 worker 中；项目不虚假宣称每一个底层系统调用都是 non-blocking。这个边界也是后续继续迁移异步数据库访问时的明确演进点。

### 从浏览器到内部 RPC 的清晰边界

- **唯一公网入口**：只有 OpenResty 暴露 80/443，业务、管理和 Judge 服务都位于内部 Docker 网络。
- **JSON/Protobuf 双协议桥接**：浏览器使用易调试的 JSON API，服务内部使用强类型 Protobuf 和 brpc。
- **Schema 驱动校验**：Gateway 根据 Protobuf descriptor 校验字段类型、嵌套对象、repeated 字段和未知参数。
- **浏览器安全的 64 位 ID**：Protobuf int64/uint64 在 JSON 中统一使用字符串，避免 JavaScript 精度丢失。
- **路由即策略**：每条路由声明 public/user/admin/super-admin 权限、CSRF 要求及 path/query/body 绑定。
- **内部方法显式拒绝**：Judge callback、缓存内部操作和管理写操作不会被普通浏览器路由意外暴露。

### 面向可靠交付的异步判题

- **Transactional Outbox**：submission/custom task 与 Judge outbox 在同一个 MySQL 事务中落库，避免“数据库成功但消息丢失”。
- **Publisher Confirm**：outbox publisher 使用 RabbitMQ confirmed publish，失败任务保留并可重试。
- **Judge Inbox 幂等**：以 message fingerprint、claim token 和 completed result 防止消息重投造成重复判题或重复写结果。
- **Retry + DLX**：瞬时失败进入分级 retry queue，达到上限后进入 dead-letter queue，错误不会静默丢失。
- **签名回调**：Judge 结果通过内部 brpc 回调，并使用 key ID/secret 校验来源。
- **提交与完成解耦**：HTTP 接入吞吐和 Judge 实际完成能力可以分别扩展、监控和限流。

### 不可信代码隔离

- 编译与运行使用独立 Docker 镜像和容器阶段。
- runner 默认禁用网络、限制 CPU/内存/超时并使用只读根文件系统。
- 执行过用户代码的 runner 会立即销毁，不跨提交复用不可信状态。
- Judge 只通过 Docker socket 管理 sandbox，业务容器不获得 Docker 权限。
- sandbox 超时、退出码、stderr、内存和运行时间都会进入结构化 Judge 结果。

### 有界资源和可预期过载

- **BusinessExecutor 有界队列**：业务处理与 brpc I/O 解耦，队列满时快速返回，而不是无界堆积。
- **并发上限可配置**：brpc threads、max concurrency、business workers、Judge workers 和 MQ prefetch 均有明确配置。
- **完整饱和指标**：记录 accepted、active、queue occupancy、rejected、queue wait 和 execution latency。
- **Gateway 连接调优**：包含 worker FD、listener backlog、reuseport、keepalive 和 upstream connection pool 配置。
- **容器安全默认值**：应用容器 read-only、drop all capabilities、`no-new-privileges`、受限 tmpfs 和内部网络。

### 缓存不仅是“加一个 Redis”

- 用户、题目、题解、评论、回复、互动状态和静态页面采用分业务 Cache-Aside。
- Jittered TTL 避免大量 key 同时失效。
- 列表使用 Redis version key，写操作通过版本递增让旧缓存自然失效，降低批量删除竞争。
- 对评论回复计数等路径使用批量查询，避免 N+1。
- 管理后台能够查看分业务缓存命中率和后端延迟。

### 安全与管理治理

- 用户 session 与 admin session 分离，管理员 session 会再次校验数据库角色，避免陈旧权限继续生效。
- Origin + CSRF cookie + header 三部分保护所有浏览器写请求。
- admin 与 super-admin 分级，测试用例、管理员账户和缓存指标等敏感能力进一步收紧。
- 管理操作进入独立有界审计队列并持久化，可按动作和时间查询。
- Gateway 到 backend 使用 Docker secret 中的独立认证 token。
- Compose 敏感配置集中在 mode 600 的 `docker/.env` 和 secret 文件，不依赖调用者 shell 环境。

### 可观测、可压测、可复盘

- spdlog 异步日志包含 flush/shutdown 跟踪，降低退出时日志丢失风险。
- 延迟监控支持采样、批量写入、最大队列、文件大小和保留数量上限。
- BusinessExecutor、缓存、Judge coroutine 和 RabbitMQ 队列均有可用于定位瓶颈的数据。
- 提供 k6、wrk2、vmstat、pidstat、sar 和 RabbitMQ 队列采样组成的完整远程压测工具链。
- 远程 bundle 能生成真实用户/admin session、CSRF 和 polling fixture，覆盖 40 条静态、API、互动、管理和 Judge 路由。
- 仓库保留有效测试、无效测试、问题定位和优化过程，不用单个“漂亮数字”替代可复现证据。

### 工程交付能力

- Protobuf IDL、生成代码和 contract test 共同维护跨服务契约。
- MySQL schema 支持首次初始化和增量 migration；已执行 migration 的 checksum 变化会阻止静默篡改。
- ODB models、Repository/Model 层和现有业务模型可渐进迁移，而不是一次性重写全部数据访问。
- 应用 artifact 包含二进制、非系统动态库、架构 metadata 和 SHA256SUMS，镜像构建会验证依赖完整性。
- 前端 JSON client 统一处理 CSRF、错误结构、旧路径映射、64 位 ID 和 Judge polling。
- 单元、契约、Repository、Legacy contract、Judge integration、direct E2E、安全和性能测试按依赖分层组织。

### 已有真实性能证据

- 缓存题目列表已验证 **1,000 VU / 约 500 RPS / 2 分钟 / 100% 成功**。
- corrected mixed 已验证 **100 VU / 188.24 RPS / 0 个 5xx / P99 163.026 ms**。
- 测试不仅记录延迟和成功率，还记录压测机 CPU/RSS、Tailscale、网络、服务饱和与 RabbitMQ ready/unacked。
- 已明确识别单机 Judge 约 1.8 task/s 的容量边界，并证明同机盲目增加 worker 不会提高吞吐。
- 对 10,000 连接测试明确标记“压测端先饱和、结论无效”，避免将客户端瓶颈包装成服务能力。

## 功能全景

| 角色/领域 | 已提供能力 |
|---|---|
| 普通访客 | 浏览题目、题解、评论、用户主页，提交临时代码并轮询 Judge 结果 |
| 用户认证 | 邮箱验证码注册/登录、密码登录、退出、改密、换邮箱、注销账号 |
| 用户中心 | 资料编辑、头像、统计、历史提交和个人状态 |
| 题目 | 分页、搜索、难度过滤、详情、样例测试、通过状态、可见性控制 |
| 在线编码 | ACE Editor、语言选择、代码提交、异步状态轮询、结果展示 |
| 自定义测试 | 自定义 stdin、异步执行、恢复未完成任务和 stdout/stderr 展示 |
| 题解 | 发布、编辑、删除、排序、点赞、收藏和用户操作状态 |
| 社区讨论 | 一级评论、二级回复、@用户、分页、点赞、收藏、编辑和删除 |
| Admin | Overview、用户列表/详情、题目 CRUD、审计日志、诊断文件 |
| Super Admin | 测试用例 CRUD、管理员账户 CRUD、缓存指标和敏感管理能力 |
| Judge 运维 | outbox、inbox 幂等、retry、DLX、队列深度、worker/prefetch 调节 |
| 数据运维 | 首次 schema 初始化、增量 migration、checksum 校验、持久化 volume |

## 适用场景

- 学校课程、实验室、小型比赛和个人 OJ 部署。
- 学习 C++20 coroutine、brpc、Protobuf、RabbitMQ 和 Docker sandbox 的完整案例。
- 学习 transactional outbox、consumer inbox、幂等、重试和最终一致性的工程实现。
- 学习 OpenResty API Gateway、CSRF/RBAC、内部网络和 secret 管理。
- 学习如何从同机压测迁移到独立负载生成器，并严谨区分客户端与服务端瓶颈。
- 作为题目、社区、管理后台和 Judge 已打通的二次开发基础。

## 设计取舍

| 取舍 | 原因 |
|---|---|
| 浏览器使用 JSON，内部使用 Protobuf/brpc | 兼顾前端开发体验、契约强度和内部调用效率 |
| 提交先持久化再异步判题 | 防止 HTTP 生命周期、MQ 短暂故障导致任务丢失 |
| 有界队列而不是无限排队 | 超载时快速、可观测地失败，保护数据库和缓存 |
| 一次性 runner 而不是复用 | 隔离不可信代码留下的进程、文件和运行状态 |
| 缓存版本递增而不是批量删除 | 降低失效竞争并让旧 key 自然过期 |
| 保留失败和无效压测记录 | 让容量结论可以复核，不隐藏方法学问题 |

## 项目状态

当前部署采用微服务入口与异步判题架构：

- OpenResty 是浏览器流量的唯一入口，对外仅开放 80/443。
- Gateway 将浏览器 JSON 请求转换为 Protobuf，通过 brpc 调用内部服务。
- `oj_server` 处理用户、题目、题解、评论、提交和 Judge 回调。
- `oj_admin` 提供独立的管理 RPC 服务，但不直接暴露公网端口。
- `judge_service` 从 RabbitMQ 消费任务，在 Docker sandbox 中编译和运行代码。
- MySQL 保存业务数据和 Judge outbox，Redis 保存缓存、会话和临时 Judge 状态。
- 已移除旧 `compile_server` 集群；当前判题完全由 RabbitMQ + `judge_service` + Docker 完成。

## 架构

```text
                           Browser
                              |
                         HTTPS :443
                              |
                    +---------v---------+
                    |     OpenResty     |
                    | static / CSRF     |
                    | JSON <-> Protobuf |
                    +----+----------+---+
                         |          |
                  brpc   |          | brpc
                         |          |
                +--------v--+    +--v----------+
                | oj_server |    |  oj_admin   |
                |   :8080   |    |   :8090     |
                +--+---+----+    +---+----------+
                   |   |             |
             +-----+   +------+      |
             |                |      |
        +----v----+      +----v------v+
        |  Redis  |      |    MySQL   |
        +---------+      +------+-----+
                                |
                         transactional outbox
                                |
                         +------v------+
                         |  RabbitMQ   |
                         +------+------+
                                |
                         +------v-------+
                         | judge_service|
                         |    :8082     |
                         +------+-------+
                                |
                       /var/run/docker.sock
                                |
                  +-------------v-------------+
                  | compiler / runner sandbox |
                  +-------------+-------------+
                                |
                         signed brpc callback
                                |
                            oj_server
```

除 OpenResty 外，所有服务只加入内部 Docker 网络。Judge sandbox 通过宿主机 Docker socket 创建，执行容器默认禁用网络并设置 CPU、内存和超时限制。

## 技术栈

| 层 | 技术 |
|---|---|
| 核心服务 | C++20、brpc、Protobuf |
| 外部网关 | OpenResty、LuaJIT、lua-protobuf、lua-resty-http |
| 消息队列 | RabbitMQ 3.13、AMQP-CPP |
| 异步 Judge | C++20 coroutine、libevent、Docker Engine API |
| 数据库 | MySQL 8.0、ODB MySQL、现有 Model/Repository 层 |
| 缓存与会话 | Redis 7.4、redis-plus-plus、hiredis |
| 前端 | 静态 HTML/CSS/JavaScript、ACE Editor、Editor.md |
| 日志与指标 | spdlog、异步日志、采样延迟 CSV、BusinessExecutor 指标 |
| 构建 | CMake 3.28.3+、protoc、Docker Compose |

根 CMake 默认标准仍为 C++17，但 `oj_server`、`oj_admin`、ODB models 和 `judge_service` 目标显式使用 C++20。

## 核心能力

### Gateway 与安全边界

- JSON 到 Protobuf 的类型校验和 64 位整数字符串保护。
- 用户、管理员、super-admin 和内部 RPC 路由分级。
- mutating 请求的 Origin、CSRF cookie 和 `X-CSRF-Token` 校验。
- Gateway 到后端使用独立认证 token。
- Cookie 支持 Secure、HttpOnly 和 SameSite 策略。
- OpenResty upstream keepalive、FD 和 listener backlog 已针对并发连接调优。

### 业务服务

- 邮箱验证码注册和登录、密码登录、密码与邮箱安全操作。
- 题目列表、详情、样例测试和通过状态。
- 题解发布、编辑、点赞和收藏。
- 两级评论、回复、点赞和收藏。
- 用户资料、头像、统计和提交记录。
- 有界 BusinessExecutor：队列满时快速失败，并记录 queue wait、execution time 和 reject 指标。

### 缓存

- Redis Cache-Aside。
- Jittered TTL，降低缓存同时失效风险。
- 列表版本号失效，避免大范围同步删除。
- 用户、题目、题解、评论、互动和静态页面缓存。
- 缓存命中和后端延迟指标可由管理后台读取。

### 异步判题

```text
HTTP submission
  -> MySQL submission + outbox transaction
  -> outbox publisher
  -> RabbitMQ oj.judge.task
  -> Judge inbox idempotency claim
  -> compiler container
  -> fresh runner container
  -> signed callback to oj_server
  -> result persistence
  -> RabbitMQ ack
```

- submission 和 outbox 在同一数据库事务中创建。
- Judge inbox 使用 message fingerprint 和 claim token 防止重复处理。
- RabbitMQ 支持 retry queue 和 DLX。
- 每个执行过不可信代码的 runner 都会销毁，不跨提交复用。
- Judge worker 和 MQ prefetch 可通过 `OJ_JUDGE_WORKERS`、`OJ_JUDGE_PREFETCH` 配置，范围为 1-64，当前基线为 4/4。

### 管理后台

- 入口：`/admin/login.html`。
- Dashboard 和服务版本。
- 用户列表与详情、题目 CRUD、测试用例和管理员账户管理。
- 缓存指标、审计日志和诊断文件查看。
- 测试用例、管理员账户和缓存指标等敏感操作要求 super-admin。

## 目录结构

```text
OnlineJudge/
├── gateway/                         OpenResty JSON/Protobuf Gateway
│   ├── routes.lua                   路由、鉴权和字段绑定
│   ├── nginx.conf                   worker、连接和 Lua 配置
│   └── lua/                         router、codec、CSRF、brpc upstream
├── src/
│   ├── oj_server/                   用户业务服务和管理服务
│   │   ├── include/control/         Control 层
│   │   ├── include/model/           Model 层
│   │   ├── include/rpc/             Protobuf/brpc adapters
│   │   └── include/runtime/         BusinessExecutor 和运行时上下文
│   ├── judge_service/               Coroutine Judge 和 Docker sandbox
│   ├── comm/                        日志、MQ、JWT、ODB、配置和共享工具
│   │   ├── models/                  ODB models
│   │   └── proto/                   Protobuf IDL 与生成代码
│   └── wwwroot/                     静态前端
├── docker/                          Compose 与应用/sandbox 镜像
├── sql/                             基础 schema 与 migrations
├── scripts/                         migration 和容器 artifact 脚本
├── test/                            单元、集成、安全和性能测试
├── docs/benchmark/                  性能计划、记录和优化过程
└── API.md                           完整 REST API 参考
```

## 快速部署

### 1. 前置条件

- Ubuntu 24.04 或兼容 Linux 环境。
- Docker Engine 和 Docker Compose plugin。
- 原生构建需要 CMake 3.28.3+、GCC/Clang C++20、Protobuf/protoc、brpc、gflags、LevelDB、OpenSSL、libevent、AMQP-CPP、redis-plus-plus、hiredis、jsoncpp、ctemplate、spdlog、libcurl、ODB 和 ODB MySQL。
- Judge 需要访问 `/var/run/docker.sock`。

### 2. 配置 Compose 环境

```bash
cp docker/compose.dev.env docker/.env
openssl rand -hex 32 > docker/dev-gateway-auth-token
chmod 600 docker/.env docker/dev-gateway-auth-token
```

修改 `docker/.env`：

- 替换所有开发密码和 `OJ_JUDGE_CALLBACK_SECRET`。
- 将 `DOCKER_GID` 设置为宿主 Docker socket 的 group ID：

```bash
stat -c '%g' /var/run/docker.sock
```

- 开发环境可以保留 `OJ_REQUIRE_TLS_SECRETS=false`，Gateway 会生成一天有效的 localhost 自签名证书。
- 生产环境必须设置真实 `OJ_TLS_CERT_PATH`、`OJ_TLS_KEY_PATH` 并启用 TLS secret 要求。
- `OJ_ALLOWED_ORIGINS` 必须列出实际浏览器 Origin。

`docker/compose.sh` 会清除宿主 shell 中可能覆盖配置的 `OJ_*`、MySQL、Redis 和 RabbitMQ 变量，并始终以 `docker/.env` 为权威配置。

### 3. 原生构建

```bash
cmake -S . -B build \
  -DOJ_TESTS=tests -DOJ_QUESTIONS=questions -DOJ_USERS=users \
  -DHOST=localhost -DUSER=oj_server -DPASSWD=password \
  -DMYOJ=myoj -DPORT=3306 \
  -DREDIS=tcp://127.0.0.1:6379

cmake --build build -j"$(nproc)"
```

二进制输出到 `output/`。CMake configure 会由 `config.in.h` 生成 `src/comm/config.h`；需要修改默认配置时应修改模板。

### 4. 打包应用镜像 artifact

应用 Dockerfile 从 `dist/phase8/` 复制二进制，原生构建后必须先刷新 artifact：

```bash
OJ_BUILD_DIR="$PWD/build" \
OJ_BUILD_JOBS="$(nproc)" \
./scripts/build_container_artifacts.sh
```

脚本会复制 `oj_server`、`oj_admin`、`judge_service` 和非系统动态库，生成 metadata 与 SHA256SUMS，并检查缺失依赖。

### 5. 构建 Judge 镜像

```bash
docker build -t oj-compiler:latest -f docker/Dockerfile.compiler docker/
docker build -t oj-runner:latest -f docker/Dockerfile.runner docker/
docker build -t oj-sandbox:latest -f docker/Dockerfile.sandbox docker/
```

### 6. 启动

```bash
./docker/compose.sh up -d --build
./docker/compose.sh ps
curl -k https://localhost/healthz
```

首次创建 MySQL volume 时会按 `docker-entrypoint-initdb.d` 顺序加载基础 schema。每次 Compose 启动时，`mysql_migrate` 还会通过 `scripts/apply_migrations.sh` 检查并应用未执行 migration；已执行 migration 的 SHA256 变化会使启动失败。

浏览器入口：

- 用户站点：`https://localhost/`
- 管理登录：`https://localhost/admin/login.html`
- 健康检查：`https://localhost/healthz`

## 常用运维命令

```bash
# 服务状态
./docker/compose.sh ps

# 查看业务和 Judge 日志
./docker/compose.sh logs -f oj_server oj_admin judge_service openresty

# 查看 RabbitMQ 队列
./docker/compose.sh exec -T rabbitmq \
  rabbitmqctl list_queues name messages_ready messages_unacknowledged consumers

# 查看 migration
./docker/compose.sh logs mysql_migrate

# 只重建单个服务，避免重建基础设施依赖
./docker/compose.sh up -d --no-deps --force-recreate judge_service
```

不要用 `--force-recreate` 同时重建整个依赖图。RabbitMQ 的持久化 nodename/hostname 固定工作仍列在性能优化计划中。

## 验证

仓库当前没有统一 CTest 注册，使用最窄的可执行目标验证修改。

```bash
# Protobuf contract
cmake --build build --target proto_contract_test
./output/proto_contract_test

# Judge 无服务 smoke
cmake --build build --target verify_log_output
./output/verify_log_output

# 日志与延迟组件
cmake --build build --target logger_test latency_monitor_test
./output/logger_test
./output/latency_monitor_test

# 前端 JSON API client 与 polling
npm --prefix src/wwwroot test
```

需要 MySQL 的 ODB/repository 测试、需要 RabbitMQ 的 Judge integration test，以及需要 Docker Judge 镜像的 direct E2E test，运行前必须先启动对应基础设施。

## 性能基线

测试环境：服务宿主机 4 vCPU/7.7 GiB，独立云压测机 2 vCPU/1.9 GiB，通过 Tailscale HTTPS 访问。

| 场景 | 结果 |
|---|---|
| 缓存题目列表 | 1,000 VU、约 500 RPS、2 分钟、59,268/59,268 成功 |
| 缓存题目列表延迟 | P50 26.912 ms、P90 36.385 ms、P99 660.419 ms；P99 含启动连接长尾 |
| corrected mixed | 100 VU、188.24 RPS、3 分钟、33,965 个 2xx、0 个 5xx、12 个设计内 429 |
| mixed 延迟 | P50 29.821 ms、P90 58.029 ms、P99 163.026 ms、P99.9 5,989.101 ms |
| 单机 Judge | 约 1.8 task/s，是当前主要容量瓶颈 |
| 10,000 连接 | 压测机先饱和，服务端容量尚未得到有效验证 |

当前判断：HTTP 缓存读取和约 200 RPS 混合业务已达到中低流量部署要求；持续高于约 1.8 task/s 的判题流量需要独立扩展 Judge 节点。

详细资料：

- [完整 API 参考](API.md)

## 远程压力测试

准备带有效 session fixture 的云端 bundle：

```bash
OJ_PUBLIC_TARGET=https://oj.example.com \
OJ_LOADGEN_IP=203.0.113.10 \
OJ_REMOTE_DEST=ubuntu@203.0.113.10:/home/ubuntu/ \
bash test/performance/remote/prepare-local.sh
```

一键部署、运行并下载结果：

```bash
OJ_PUBLIC_TARGET=https://oj.example.com \
OJ_REMOTE_HOST=ubuntu@203.0.113.10 \
OJ_SSH_KEY="$HOME/.ssh/cloud.pem" \
OJ_RUN_STAGES=health,questions100,questions1000,connections10000,mixed \
bash test/performance/remote/deploy-and-run.sh
```

fixture 包含有效用户/admin session，权限为 600，默认七天过期。不要提交或分享生成的 archive。完整说明见 [远程压测 README](test/performance/remote/README.md)。

## API

主要 JSON 路由：

| 领域 | 示例 |
|---|---|
| CSRF | `GET /api/csrf`、`GET /admin/api/csrf` |
| 认证 | `POST /api/auth/register`、`POST /api/auth/login/password`、`POST /api/auth/logout` |
| 用户 | `GET /api/user`、`GET /api/users/:uid`、`PATCH /api/user/profile` |
| 题目 | `GET /api/questions`、`GET /api/questions/:question_id` |
| 题解 | `GET/POST /api/questions/:question_id/solutions`、`POST /api/solutions/:solution_id/like` |
| 评论 | `GET/POST /api/solutions/:solution_id/comments`、`GET /api/comments/:comment_id/replies` |
| Judge | `POST /api/questions/:question_id/submissions`、`GET /api/submissions/:submission_id` |
| 自定义测试 | `POST /api/questions/:question_id/custom-tests`、`GET /api/custom-tests/:task_id` |
| 管理后台 | `/admin/api/*`，按 admin/super-admin 权限保护 |

所有浏览器调用都应通过 OpenResty JSON Gateway，不应直接调用 brpc backend。字段、响应结构、状态码和完整路由见 [API.md](API.md)。

## 已知限制

- 单个 4C Docker Judge 节点约 1.8 task/s；更高持续到达率会使 RabbitMQ 积压。
- 远程 mixed P99.9 仍有约 6 秒长尾，正在通过逐路由延迟和 upstream timing 定位。
- 10,000 连接结果受 2C2G 压测机限制，需要更强或分布式生成器重新测试。
- 当前 mixed polling 使用预生成 target；端到端“入队到完成”SLO 测试仍需补充。
- RabbitMQ 固定 nodename、Judge 水平扩容、Docker 阶段耗时分解和长时间 soak 仍在计划中。
