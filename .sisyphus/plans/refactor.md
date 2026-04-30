# 代码重构优化方案

## TL;DR

> **目标**: 将 2438 行的 `oj_control.hpp` 和 3496 行的 `oj_model.hpp` 拆分为 6+ 个功能模块
> **原则**: 每个模块 ≤500 行，单一职责，低耦合
> **风险**: 低——纯文件拆分，不改业务逻辑

---

## 现状分析

| 文件 | 行数 | 职责 |
|------|:--:|------|
| `oj_control.hpp` | 2438 | 用户/题目/题解/评论/缓存/Auth/邮件/判题 — **全部混在一起** |
| `oj_model.hpp` | 3496 | 所有 DB 查询 + Redis 缓存 — **单一类 40+ 方法** |
| `oj_cache.hpp` | 846 | Redis 操作 — **已独立，合理** |
| `oj_view.hpp` | 115 | 模板渲染 — **合理** |
| `oj_session.hpp` | 261 | 会话 — **合理** |
| `oj_mail.hpp` | 250 | 邮件 — **合理** |

## 问题

1. **oj_control.hpp** 是「神类」——一个类包含 50+ 方法，覆盖全部业务
2. **oj_model.hpp** 是「万能仓库」——所有 SQL 在一个类中
3. 修改评论功能需要在一大堆用户/题目/题解代码中找到评论相关部分
4. 两个文件都是 header-only，任何改动触发全量重编译

---

## 拆分方案

### Model 层拆分

```
oj_model.hpp (3496 行)
├── model_base.hpp        (~100 行)  Model 基类，CreateConnection, QueryCount
├── model_question.hpp    (~400 行)  题目 CRUD + 缓存
├── model_solution.hpp    (~600 行)  题解 CRUD + 列表缓存 + 点赞收藏
├── model_comment.hpp     (~800 行)  评论 CRUD + 嵌套回复 + 缓存
├── model_user.hpp        (~600 行)  用户 CRUD + 统计 + 提交
├── model_admin.hpp       (~400 行)  管理员查询
└── model_auth.hpp        (~300 行)  鉴权/密码
```

### Controller 层拆分

```
oj_control.hpp (2438 行)
├── control_base.hpp      (~100 行)  Control 基类，通用工具(Judge/GetEffectiveAvatarUrl)
├── control_auth.hpp       (~300 行)  注册/登录/验证码
├── control_question.hpp   (~300 行)  题目列表/详情/搜索
├── control_solution.hpp   (~400 行)  题解 CRUD/列表/点赞
├── control_comment.hpp    (~400 行)  评论 CRUD/嵌套/点赞
├── control_user.hpp       (~300 行)  用户信息/设置/头像
└── control_admin.hpp      (~400 行)  管理面板(已有 oj_admin.hpp)
```

### 预期效果

| 指标 | 重构前 | 重构后 |
|------|:--:|:--:|
| 最大文件行数 | 3496 | ≤800 |
| Model 方法数 | 40+ | 每模块 5-10 |
| Controller 方法数 | 50+ | 每模块 5-8 |
| 增量编译 | 全量重编 | 仅修改模块重编 |
| 新人理解成本 | 需要读 3000 行 | 读对应模块 300 行 |

---

## 实施策略

### 阶段 1: Model 拆分（低风险）

1. 创建 `model_base.hpp` — 提取 `CreateConnection()`, `QueryCount()`, `QueryUser()`, `ExecuteSql()`
2. 创建 `model_comment.hpp` — 提取所有 `GetCommentsBySolutionId`, `CreateComment`, `DeleteComment`, `GetCommentReplies`, `ToggleCommentAction` 等
3. 创建 `model_solution.hpp` — 提取 `GetSolutionsByPage`, `GetSolutionById`, `ToggleSolutionAction` 等
4. 创建 `model_user.hpp` — 提取 `GetUserById`, `GetUserStats`, `UpdateUserAvatar` 等
5. 创建 `model_question.hpp` — 提取 `GetOneQuestion`, `GetQuestionsByPage` 等
6. `oj_model.hpp` 改为 `#include` 所有子模块的聚合头文件

### 阶段 2: Controller 拆分（需配合路由调整）

1. 创建 `control_comment.hpp` — `CommentController` 类，提取 `PostComment`, `GetTopLevelComments`, `GetCommentReplies`, `EditComment`, `DeleteComment`
2. 创建 `control_solution.hpp` — `SolutionController` 类
3. 创建 `control_auth.hpp` — `AuthController` 类
4. `oj_control.hpp` 改为聚合，路由注册保持不变

### 接口优化

1. **Model 基类抽象**: 每个子 Model 继承 `ModelBase`，共享 `CreateConnection`
2. **统一错误码**: 定义 `enum class ErrorCode` 替代字符串 `"DB_ERROR"` / `"NOT_FOUND"`
3. **统一响应构建**: `HttpUtil::BuildSuccess(result)` / `HttpUtil::BuildError(code, msg)`

---

## 风险与注意事项

1. **include 依赖**: 当前代码大量使用 `#include "../../comm/comm.hpp"` 的相对路径，重构后需统一为项目根路径
2. **namespace**: 目前只有一个 `namespace ns_model` 和 `namespace ns_control`，拆分后子模块可以继续用同一 namespace，无需改动调用方
3. **header-only**: 保持 header-only 模式，不引入 .cpp 文件
4. **CMakeLists.txt**: 无需修改——头文件变化不影响编译目标
5. **每次拆分后构建验证**: `make -j4` 确保无编译错误
