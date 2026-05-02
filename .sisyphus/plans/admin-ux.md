# oj_admin 改造开发文档

## TL;DR

> **目标**: 统一管理后台 UI 风格、修复缓存命中率显示、增加测试用例编辑功能
> **改动量**: ~6 个文件（前端 3 + 后端 3），约 200 行变更

---

## Requirement 1: UI 统一

### 1.1 壁纸更换

`admin/index.html` 当前使用纯 CSS 渐变背景（`radial-gradient`），需改为主站一致的毛玻璃暗色主题。

### 1.2 毛玻璃风格统一

| 主站特征 | 实现方式 |
|----------|---------|
| 背景 | `background-image: url('/pictures/preview.jpg')` + `background-attachment: fixed` |
| 模糊 | `.background { filter: blur(0.2px); position: fixed; z-index: -1; }` |
| 面板 | `background: rgba(255, 255, 255, 0.08); backdrop-filter: blur(20px)` |
| 边框 | `border: 1px solid rgba(244, 246, 240, 0.1)` |
| 色彩 | `#2D2F2C` 底色, `#F4F6F0` 文字, `#4CAF50` 强调 |

**修改文件**: `admin/index.html` — 替换 body 背景和面板样式，添加背景图层和雨线效果。

### 1.3 雨线效果

所有主站页面有 CSS rain-lines 动画（`@keyframes rain`）。`admin/index.html` 需添加相同的 HTML 结构 + CSS + JS（`createRainLines()`）。

---

## Requirement 2: 缓存命中信息修复

### 2.1 现状分析

`CacheMetricsSnapshot` 结构体（`oj_model.hpp:293`）记录了 5 类指标：

```cpp
struct CacheMetricsSnapshot {
    // List cache (题解/用户列表)
    int64_t list_requests, list_hits, list_misses, list_db_fallbacks, list_total_ms;
    // Detail cache (题目/题解详情)
    int64_t detail_requests, detail_hits, detail_misses, detail_db_fallbacks, detail_total_ms;
    // HTML static (静态页面)
    int64_t html_static_requests, html_static_hits, html_static_misses;
    // HTML list (列表页面)
    int64_t html_list_requests, html_list_hits, html_list_misses;
    // HTML detail (详情页面)
    int64_t html_detail_requests, html_detail_hits, html_detail_misses;
};
```

### 2.2 缺失的指标记录点

**问题**: `Record*Metrics` 方法只在少数路径被调用：
- `RecordListMetrics` — 在 `GetQuestionsByPage`、`GetUsersPaged` 中有调用
- `RecordDetailMetrics` — 在 `GetOneQuestion` 中有调用
- `RecordHtmlStaticMetrics` — 在 `GetStaticHtml` 中有调用
- `RecordHtmlListMetrics` — 在 `AllQuestions` 中部分调用
- `RecordHtmlDetailMetrics` — 在 `OneQuestion` 中调用

**但是**: 评论缓存（`GetCommentsBySolutionId`）、回复缓存（`GetCommentReplies`）、题解缓存（`GetSolutionsByPage`）、用户缓存（`GetUserById`）**均未记录指标**。

### 2.3 修复方案

| 模块 | 需添加的 RecordMetrics 调用 |
|------|--------------------------|
| `GetCommentsBySolutionId` | `RecordListMetrics(cache_hit, db_fallback, cost_ms)` |
| `GetCommentReplies` | `RecordListMetrics(...)` |
| `GetSolutionsByPage` | `RecordListMetrics(...)` |
| `GetSolutionById` | `RecordDetailMetrics(...)` |
| `GetUserById` | `RecordDetailMetrics(...)` |
| model 层所有缓存读路径 | 在缓存命中/未命中处添加指标记录 |

### 2.4 前端展示修复

`admin/index.html` 中 `admin-app.js` 已有缓存面板渲染逻辑，但需要确认：
1. 百分比显示是否正确格式化（`CalcRate` 函数已就绪）
2. 数据从 `/api/admin/overview` 正确获取

---

## Requirement 3: 题目板块调整

### 3.1 描述框扩大

`admin/index.html` 中 `#q-desc` textarea 的 `rows` 属性或 CSS `height` 需调整为使下边界对齐屏幕底部。

CSS 修改：
```css
#q-desc {
    min-height: calc(100vh - 300px); /* 动态填满可用空间 */
    resize: vertical;
}
```

### 3.2 测试用例编辑

#### 3.2.1 新增 API

| 路由 | 方法 | 功能 |
|------|------|------|
| `GET /api/admin/questions/(\d+)/tests` | GET | 获取题目所有测试用例 |
| `POST /api/admin/questions/(\d+)/tests` | POST | 新增测试用例 |
| `PUT /api/admin/tests/(\d+)` | PUT | 编辑测试用例 |
| `DELETE /api/admin/tests/(\d+)` | DELETE | 删除测试用例 |

#### 3.2.2 后端实现（`oj_admin.cpp`）

```cpp
// GET tests for a question
svr.Get(R"(/api/admin/questions/(\d+)/tests$)", [...](...) {
    std::string qid = req.matches[1];
    Json::Value tests;
    model->Question().GetSampleTestsByQuestionId(qid, &tests);
    // ...
});

// POST new test
svr.Post(R"(/api/admin/questions/(\d+)/tests$)", [...](...) {
    // Insert into tests table: question_id, in, out, is_sample
});
```

#### 3.2.3 Model 层 (`model_question.hpp`)

新增方法：
```cpp
bool SaveTest(const std::string& question_id, const Test& test);
bool UpdateTest(int test_id, const Test& test);
bool DeleteTest(int test_id);
```

#### 3.2.4 前端 (`admin-app.js` + `admin/index.html`)

在题目编辑面板中添加：
1. "编辑测试用例"按钮（与"编辑题目"按钮并排）
2. 测试用例列表（表格形式：ID、输入、输出、是否样例）
3. 每个用例旁有"编辑"/"删除"按钮
4. "新增测试用例"按钮 → 弹出表单
5. 编辑/新增表单包含：输入（textarea）、输出（textarea）、是否样例（checkbox）

#### 3.2.5 样式（测试用例表格带滚动）

```css
.test-case-table-container {
    max-height: 400px;
    overflow-y: auto;
    border: 1px solid var(--border);
    border-radius: 8px;
}
```
