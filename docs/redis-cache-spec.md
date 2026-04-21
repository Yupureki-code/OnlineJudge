# OnlineJudge Redis 缓存规范（v1）

## 1. 目标与范围

本规范用于指导 OnlineJudge 在不改变 MySQL 真相源地位的前提下，引入 Redis 作为读性能加速层与会话共享层。

目标：
- 降低高频读接口的 MySQL 压力。
- 保证 Redis 异常时系统可自动降级回源。
- 统一 key 命名、TTL、失效策略，避免后续维护混乱。

非目标：
- 不在 v1 中引入“Redis 先写后回写 MySQL”的最终一致复杂链路。
- 不在 v1 中把 Redis 作为业务数据唯一存储。

---

## 2. 设计原则

1. MySQL 是真相源，Redis 仅做缓存与加速。
2. 所有缓存读取都必须支持 Cache Miss 回源 DB。
3. Redis 访问失败必须旁路，不影响主业务返回。
4. 所有 TTL 需加随机抖动，避免雪崩。
5. 必须支持空值缓存，防止缓存穿透。
6. key 命名必须版本化，支持未来平滑迁移。

---

## 3. key 命名规范

统一格式：

`oj:{env}:v{version}:{domain}:{entity}:{id_or_params}`

字段说明：
- `oj`：固定业务前缀。
- `{env}`：环境，如 `dev`、`test`、`prod`。
- `v{version}`：缓存协议版本，如 `v1`。
- `{domain}`：业务域，如 `question`、`session`、`rate`。
- `{entity}`：实体类别，如 `detail`、`list`、`status`。
- `{id_or_params}`：主键或参数摘要。

示例：
- `oj:dev:v1:question:detail:1001`
- `oj:dev:v1:question:list:1:20:dp:kw_8f14c2`
- `oj:dev:v1:session:user:7f8abc...`

参数类 key 要求：
- 使用固定顺序拼接参数。
- 搜索词等长文本参数使用哈希摘要（建议 sha1 前 6-8 位）防止 key 过长。

---

## 4. 数据结构选型规范

## 4.1 题目详情缓存（one_question）

- 类型：`String`（JSON）
- key：`oj:{env}:v1:question:detail:{qid}`
- TTL：`3600 ± 300` 秒
- 值：题目完整 JSON（标题、难度、描述、模板代码、限制信息等）

建议字段：
- `id`
- `title`
- `difficulty`
- `description`
- `header`
- `tail`
- `updated_at`
- `version`

原因：
- 读多写少，整对象读取。
- 实现简单，序列化成本低。

## 4.2 题目全局索引（排序基准）

- 类型：`ZSet`
- key：`oj:{env}:v1:question:index:all`
- member：题目 `id`
- score：`id` 或创建时间戳（二选一，要求全局稳定排序）

说明：
- 作为全量题目集合与排序基准。
- 可用于后续范围查询、重建分页缓存、增量校验。

## 4.3 题目列表分页缓存（all_questions）

- 类型：`String`（JSON）
- key：`oj:{env}:v1:question:list:{query_hash}:{page}:{size}:{list_version}`
- TTL：`600 ± 120` 秒
- 用途：缓存“id/标题/难度筛选后”的某一页结果

其中：
- `query_hash`：由筛选条件标准化后计算摘要，建议包含 `id/title/difficulty`。
- `list_version`：列表版本号，用于题目变更后的批量逻辑失效。

value 建议结构：
- `query`：原筛选条件快照（便于调试与校验）
- `page`
- `size`
- `total_count`
- `total_pages`
- `items`：当前页条目（仅 `id/title/difficulty` 精简字段）
- `ids`：当前页题目 id 数组（可选）
- `list_version`
- `generated_at`

说明：
- 列表页只缓存展示所需精简字段，不缓存完整题面。
- 空结果也要缓存（短 TTL），防止缓存穿透。

## 4.4 列表版本缓存

- 类型：`String`（整数）
- key：`oj:{env}:v1:question:list:version`
- TTL：不设置（持久键）

说明：
- 题目新增/修改/删除时递增该值。
- 分页 key 带版本号后，旧分页缓存自动逻辑失效。

## 4.5 用户会话缓存

- 类型：`Hash`
- key：`oj:{env}:v1:session:user:{sid}`
- TTL：`86400` 秒（24h，滑动续期）
- 字段建议：
  - `uid`
  - `email`
  - `name`
  - `role`
  - `create_time`
  - `last_seen`

说明：
- 使用 Hash 便于局部更新（如 `last_seen`）。
- 每次访问受保护接口可续期（EXPIRE）。

## 4.6 验证码缓存

- 类型：`String`
- key：`oj:{env}:v1:verify:email:{email_hash}`
- TTL：`300` 秒
- 值：验证码（建议加签或加盐摘要）

安全建议：
- 不建议明文长时间保留验证码。
- 验证通过后立即删除 key。

## 4.7 限流计数

- 类型：`String` 计数器（`INCR`）
- key：`oj:{env}:v1:rate:{scope}:{target}:{api}:{minute}`
- TTL：`120` 秒
- 典型 scope：`ip`、`email`、`uid`

说明：
- v1 先用固定窗口实现。
- 后续可演进为滑动窗口或令牌桶。

## 4.8 点赞与收藏（预留）

- 类型：`Set`
- key：`oj:{env}:v1:like:question:{qid}`
- 成员：`uid`

常用操作：
- 点赞：`SADD`
- 取消点赞：`SREM`
- 是否点赞：`SISMEMBER`
- 点赞数：`SCARD`

## 4.9 排行榜（预留）

- 类型：`ZSet`
- key：`oj:{env}:v1:rank:ac`
- member：`uid`
- score：通过题数或综合得分

---

## 5. TTL 与抖动策略

统一规则：
- 基础 TTL 必须附加随机抖动，推荐范围 5%-15%。
- 避免同类 key 同时失效导致雪崩。

建议 TTL：
- 题目详情：3600 秒，抖动 ±300 秒
- 题目列表分页：600 秒，抖动 ±120 秒
- 会话：86400 秒（可滑动续期）
- 空值缓存：60 秒（短 TTL）

---

## 6. 失效与一致性策略

## 6.1 读路径（Cache-Aside）

流程：
1. 先读 Redis。
2. 命中直接返回。
3. 未命中查 MySQL。
4. 查到后回填 Redis（带 TTL + 抖动）。

### all_questions 专项流程（筛选 + 分页）

1. 标准化筛选参数（`id/title/difficulty`）并生成 `query_hash`。
2. 读取 `question:list:version`，组装分页 key。
3. 命中缓存：直接返回分页 value。
4. 未命中缓存：回源 MySQL 执行筛选与分页查询。
5. 将分页结果回填 Redis（TTL + 抖动）。
6. 若结果为空，也写入空分页对象（短 TTL）。

可选增强：
- 对同一分页 key 加短互斥锁（3-5 秒）防击穿。

## 6.2 写路径（更新题目等）

采用“删除缓存 + 更新 DB + 延迟二次删除”策略：
1. 删除对应题目详情 key（`question:detail:{id}`）。
2. 更新 MySQL。
3. 递增 `question:list:version`（替代全量扫描删除分页 key）。
4. 异步或延迟短时间后再次删除详情 key（兜底）。

说明：
- 避免并发读写时出现脏缓存。

## 6.3 空值缓存（防穿透）

- 对不存在数据写入特殊占位值（如 `__NULL__`）。
- TTL 建议 30-120 秒。
- 读取占位值时直接返回“未找到”，不查 DB。

---

## 7. 容灾与降级规范

1. 所有 Redis 调用必须捕获异常（如连接失败、超时）。
2. Redis 异常时：
   - 读路径：直接回源 MySQL。
   - 写路径：继续写 MySQL，记录日志并跳过缓存操作。
3. 不允许 Redis 故障导致接口 500（除非 DB 同时不可用）。
4. 必须记录指标：
   - 缓存命中率
   - Redis 请求耗时
   - Redis 异常次数
   - 回源 DB 次数

---

## 8. 接口映射（v1 实施优先级）

P0（先做，收益最大）：
1. `GET /all_questions` 对应“筛选分页结果缓存”。
2. `GET /question/{id}` 对应详情缓存。
3. 会话读取路径（登录态识别）迁移至 Redis。

P1（次阶段）：
1. 邮箱验证码缓存与限流。
2. 提交状态短缓存（如判题轮询状态）。

P2（后续扩展）：
1. 点赞集合。
2. 榜单 ZSet。

---

## 9. C++ 侧实现约束

建议组件：
- `oj_cache.hpp`：缓存门面（Get/Set/Delete，隐藏 Redis 细节）。
- `oj_cache_key.hpp`：统一 key 构造器，防止业务层手写 key。
- `oj_cache_metrics.hpp`：命中率与异常统计。

编码约束：
1. 业务代码不可直接拼 Redis key，必须走 key builder。
2. 业务代码不可直接依赖 redis++，必须经由 `oj_cache` 抽象。
3. 所有缓存值序列化格式统一 JSON（v1）。
4. 分页缓存值必须包含 `total_count` 与 `total_pages`，保证前端分页一致性。

---

## 10. 运维与安全建议

1. Redis 开启密码认证，生产环境禁止无认证访问。
2. 限制 Redis 监听地址，不暴露公网。
3. 分环境隔离 key 前缀（`dev/test/prod`）。
4. 控制缓存值大小（建议单值不超过 256KB）。
5. 对邮箱、验证码等敏感内容使用哈希或脱敏值作为 key 参数。

---

## 11. 验收清单

1. Redis 宕机时，题目查询与登录态接口仍可用。
2. 题目列表与详情接口平均延迟下降，且 DB 查询次数明显减少。
3. 缓存命中率可观测并达到预期（初期目标 60%+，稳定后 80%+）。
4. 更新题目后，无长期脏读。
5. 不存在 key 命名混乱或同功能多种 key 风格。

---

## 12. 变更记录

- v1（2026-04-21）：初版规范，覆盖 key 体系、结构选型、TTL、一致性与降级策略。
- v1.1（2026-04-21）：细化 question 缓存模型，新增全局索引、筛选分页 key/value 规范与版本化失效策略。

## 13. 相关文档

- 页面链路性能优化清单（不缓存整页 HTML）：`docs/page-performance-checklist.md`
