# SPA 导航栏搜索功能开发计划

## TL;DR

> **目标**: SPA 导航栏搜索框输入关键词后，跳转到 SPA 题库页面并按题目标题筛选
> **当前状态**: 搜索已重定向到旧 `/all_questions` 页面（非 SPA），SPA 题库页已有本地筛选但无外部传入关键词的入口
> **改动量**: 3 个文件，约 20 行变更

---

## Context

### 当前架构
- **SPA 路由** (`js/spa-router.js`): hash-based，`#home`/`#about`/`#profile`，调用 `window.initPage(params)`
- **导航栏搜索** (`spa/app.html:337-342`): 重定向到 `window.location.href = '/all_questions?q=keyword'`（旧页面）
- **题库页面** (`spa/pages/questions.html`): 已有 `searchBox` 和 `currentSearch` 客户端筛选，调用 `loadQuestions()` 一次性拉取全部题目再过滤

### 问题
1. 搜索跳转到旧 `/all_questions` 页面，脱离 SPA
2. SPA 题库页面没有接收外部关键词的机制

### 目标
1. 搜索 → `#questions?q=keyword` (SPA 内部路由)
2. 题库页面从 URL 参数读取关键词并自动筛选

---

## Execution Strategy

```
Wave 1 (并行):
├── Task 1: app.html — 修改 navigateToSearch 跳转到 SPA #questions
├── Task 2: spa-router.js — 支持 hash 查询参数传递给 initPage
├── Task 3: questions.html — initPage 读取 q 参数并应用筛选

Wave 2:
└── Task 4: 构建验证 + 提交
```

---

## TODOs

- [x] 1. `spa/app.html` — 修改搜索跳转逻辑

  **What to do**:
  - 修改 `navigateToSearch` 函数（line 337-342）
  - 从 `window.location.href = '/all_questions?q=' + q` 改为 `window.location.hash = '#questions?q=' + q`
  - 同时修改 `keypress` 为 `keydown`（`keypress` 已废弃）

- [x] 2. `js/spa-router.js` — 支持 hash 查询参数

  **What to do**:
  - 在 `loadPage` 函数中（line 27-77）解析 hash 中的查询参数
  - hash 格式: `#questions?q=keyword` → 提取 `{q: "keyword"}`
  - 传递给 `window.initPage(params)` 作为参数对象

- [x] 3. `spa/pages/questions.html` — 接收外部关键词

  **What to do**:
  - 修改 `initPage` 函数签名：`initPage: function(params)`
  - 从 `params` 读取 `q`，设置 `this.currentSearch = params.q || ''`
  - 将 searchBox 的 value 同步为关键词
  - 保持 `loadQuestions()` → `renderQuestions()` 流程

---

## Verification

部署后：
1. 在 SPA 导航栏搜索框输入 "A+B" → 回车
2. 页面应跳转到 `#questions?q=A%2BB`
3. 题库页面只显示标题包含 "A+B" 的题目
4. searchBox 应显示 "A+B"
