# OnlineJudge 页面链路性能优化清单（不使用 Redis 缓存 HTML）

## 1. 适用范围与结论

适用页面：
- `index.html`（当前入口已切到 `/spa/app.html`）
- `all_questions` 页面
- `one_question` 页面
- `about`、`profile` 等展示页

结论：
- 不建议把完整 HTML（如 `index.html`、`one_question.html`）放入 Redis。
- 推荐路径：进程内/OS 文件缓存 + HTTP 缓存头 + 反向代理静态加速 + 业务数据缓存（Redis）。

---

## 2. 当前链路现状（与代码对应）

1. 服务端会在请求时读取 HTML 并注入用户信息脚本：
   - `src/oj_server/src/oj_server.cpp` 中 `ReadHtmlFile`、`InjectUserInfo`、`GET /`。
2. 题目页和列表页使用模板渲染：
   - `src/oj_server/include/oj_view.hpp` 中 `AllExpandHtml`、`OneExpandHtml`。
3. 静态目录通过 mount 暴露：
   - `src/oj_server/src/oj_server.cpp` 中 `set_mount_point("/js"...)` 等。

---

## 3. 实施清单（按优先级）

## P0: 先做（低风险高收益）

### 3.1 建立基线观测
- [ ] 为以下路由增加耗时日志（至少 ms 级）：`/`、`/all_questions`、`/question/{id}`。
- [ ] 记录 `ReadHtmlFile` 耗时与模板渲染耗时（分别统计）。
- [ ] 记录 QPS、P50、P95、P99。

验收：
- [ ] 能明确拆分“读文件慢”还是“渲染慢/DB慢”。

### 3.2 启用浏览器缓存协商（ETag/Last-Modified）
- [ ] 对静态资源（`/js`、`/css`、`/pictures`）返回 `ETag`。
- [ ] 支持 `If-None-Match`，命中返回 `304`。
- [ ] 增加 `Cache-Control`：
  - 版本化静态资源：`public, max-age=31536000, immutable`
  - 非版本化资源：`public, max-age=300`

验收：
- [ ] 浏览器二次访问静态资源出现大量 304 或本地缓存命中。

### 3.3 减少重复磁盘读取（服务内缓存）
- [ ] 为固定页面（如 `about.html`）加进程内只读缓存。
- [ ] 为模板源文件加进程内缓存，避免每次重新从磁盘加载模板文件。
- [ ] 文件变更通过“重启生效”或“文件 mtime 检测失效”两种模式二选一。

验收：
- [ ] `ReadHtmlFile` 平均耗时显著下降。

---

## P1: 第二阶段（中风险中收益）

### 3.4 反向代理托管静态资源（推荐 Nginx/Caddy）
- [ ] 将 `/js`、`/css`、`/pictures` 由反向代理直接回源文件。
- [ ] 打开 `sendfile`、`gzip`（可选 brotli）。
- [ ] 给静态资源单独 access log，确认命中率和字节流量。

验收：
- [ ] C++ 主服务静态请求占比明显下降。

### 3.5 页面内容压缩
- [ ] 对 HTML 响应启用 gzip（文本类收益明显）。
- [ ] 仅压缩超过阈值的响应（例如 >1KB）。

验收：
- [ ] HTML 传输体积下降，弱网 TTFB/总耗时降低。

### 3.6 模板渲染链路微优化
- [ ] 避免重复构造大字符串（预留 `reserve`，减少拷贝）。
- [ ] 统一 HTML 注入点逻辑，避免重复拼接脚本片段。
- [ ] 如果页面布局稳定，可拆“壳模板 + 数据片段”减少全量渲染。

验收：
- [ ] `/all_questions`、`/question/{id}` 服务端渲染耗时下降。

---

## P2: 第三阶段（按需）

### 3.7 业务数据进 Redis，而不是整页 HTML
- [ ] 缓存题目详情数据：`oj:{env}:v1:question:detail:{qid}`。
- [ ] 缓存题目列表分页数据：`oj:{env}:v1:question:list:{page}:{size}:{tag}:{kw_hash}`。
- [ ] 模板仍在本地渲染，数据来自 Redis 命中。

验收：
- [ ] MySQL 查询次数下降，页面接口整体延迟下降。

### 3.8 例外场景（允许页面级缓存）
仅在以下条件同时满足时考虑“页面级缓存”：
- [ ] 页面生成 CPU 开销高（重 SSR），且命中稳定。
- [ ] 页面内容弱个性化，失效策略明确。
- [ ] 可接受短时陈旧且已定义回源策略。

---

## 4. 不建议做法（明确禁止）

- [ ] 不把所有 HTML 大文件直接塞入 Redis 作为默认方案。
- [ ] 不在没有失效策略的前提下做页面缓存。
- [ ] 不把 Redis 当真相源（真相源仍是 MySQL + 文件系统）。

---

## 5. 推荐实施顺序（两周版）

第 1-2 天：
- [ ] 接口耗时与读文件耗时埋点。
- [ ] 输出基线报告（P50/P95、QPS、静态资源请求占比）。

第 3-5 天：
- [ ] 完成 ETag/Cache-Control。
- [ ] 进程内模板/静态文件缓存（固定页面）。

第 6-8 天：
- [ ] 接入 Nginx 静态托管并启用 gzip。

第 9-12 天：
- [ ] 将题目详情/列表数据接入 Redis（按 `docs/redis-cache-spec.md`）。

第 13-14 天：
- [ ] 压测与对比报告，确认收益并收敛参数（TTL、压缩阈值）。

---

## 6. 验收指标模板

目标建议：
- [ ] `/all_questions`：P95 下降 30% 以上。
- [ ] `/question/{id}`：P95 下降 30% 以上。
- [ ] 静态资源请求中 304/本地缓存命中率 > 70%。
- [ ] Redis 宕机演练后页面接口仍可用（自动回源）。

---

## 7. 与 Redis 规范的关系

- Redis 仅缓存业务数据，不缓存整页 HTML。
- 具体 key 与 TTL 规则见：`docs/redis-cache-spec.md`。
