# Bug Fix 开发文档

## TL;DR

> **目标**: 修复 6 个问题：①题解详情刷新跳页 ②按钮 UI 统一 ③删除/编辑评论提示 ④页面加载遮罩 ⑤动态刷新 ⑥嵌套回复显示
> **改动量**: ~5 个文件

---

## Issue 1: 题解详情页刷新跳页

### 现状
`/questions/{id}/solutions/{id}` 刷新后展示的是 `solutions/detail.html`（全宽题解页），而不是原来的分栏布局（左题解+右代码）。

### 根因
该路由（我之前新增的）服务于 `solutions/detail.html`，这是独立题解页面，不是 `one_question.html` 的分栏视图。

### 修复
将路由 `GET /questions/(\d+)/solutions/(\d+)$` 改为重定向到问题详情页：
```cpp
svr.Get(R"(/questions/(\d+)/solutions/(\d+)$)", [&](const Request& req, Response& rep) {
    std::string qid = req.matches[1];
    rep.set_redirect("/questions/" + qid, 302);
});
```
用户刷新后回到问题页，可点击题解查看详情（保持原有分栏布局）。

---

## Issue 2: 按钮 UI 统一

### 现状
代码编辑区下方有 `<a href="..." class="option-btn">返回题库</a>` 和 `<a href="..." class="option-btn">发布题解</a>` 两个 `<a>` 标签按钮。

### 修复
1. 删除"发布题解"按钮（已在 `solutions/new.html` 或其他地方有入口）
2. 将"返回题库"改为 `<button>` 标签，样式与其他 option-btn 一致：
```html
<button type="button" class="option-btn" onclick="window.location.href='/all_questions'">返回题库</button>
```
在 `one_question.html` 中找到 `jump-option` div 内的这两个链接，替换。

---

## Issue 3: 删除/编辑评论提示

### 修复
在 `one_question.html` 和 `solutions/detail.html` 的编辑和删除评论函数中：
- `editDetailComment` 成功后：`showToast('编辑成功！');`
- `editDetailComment` 失败后：`showToast('编辑失败', true);`
- `deleteDetailComment` 成功后：`showToast('删除成功！');`
- `deleteDetailComment` 失败后：`showToast('删除失败', true);`

---

## Issue 4: 页面加载遮罩

### 现状
个人中心、题库搜索等动态加载页面先显示空白再渲染数据。

### 修复
利用已有的 `.page-loading-mask` 组件（存在于 `index.html`、`one_question.html` 等页面），为缺少此遮罩的页面添加：
- `user/profile.html`
- `all_questions.html`
- `solutions/detail.html`
- 其他动态加载页面

在数据 fetch 之前调用 `__showPageLoading('正在加载中...')`，在数据渲染完成后调用 `__hidePageLoading()`。

---

## Issue 5: 动态网页自动刷新

### 状态
已在之前的修复中实现——`submitDetailComment`、`submitDetailReply`、`editDetailComment`、`deleteDetailComment` 都在操作成功后调用 `loadDetailComments()` 重新加载评论区。

无需额外修改。如有个别遗漏，补充即可。

---

## Issue 6: 嵌套引用回复仍失败

### 现状
一级评论下的嵌套评论无法正常显示 @引用。

### 分析
之前已在 `renderReplyInline` 中实现了 `@reply_to_user_name` 显示。问题可能是：
1. 回复列表的 `reply_count` 为 0，折叠按钮不显示
2. `GetTopLevelComments` 中计算 `reply_count` 时 N+1 查询失败
3. 前端 CSS 或 JS 渲染问题

### 修复
检查 `GetTopLevelComments` controller 中的 `reply_count` 计算逻辑（oj_control.hpp:1316-1325）。如果 `GetCommentReplies` 调用失败（如 DB 列不存在），`reply_count` 设为 0，导致回复折叠按钮不显示。

验证后端 API `GET /api/comments/{id}/replies` 返回的 `reply_to_user_name` 字段是否正确。如果 `reply_to_user_name` 为 null 或空，检查 DB 中 `reply_to_user_id` 是否正确存储。

前端：确保 `renderReplyInline` 正确渲染 `@reply_to_user_name`（已实现，需确认 API 返回正确）。
