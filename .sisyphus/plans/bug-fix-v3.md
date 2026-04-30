# Bug 修复开发文档

## TL;DR

> **目标**: 修复 4 个审查发现的问题：①导航栏样式统一 ②评论计数删除后不更新 ③评论/回复无成功提示 ④发布题解编辑器换为 Editor.md
> **改动量**: ~10 个文件，约 80 行变更

---

## Bug 1: SPA 导航栏布局统一

### 现状
各页面 header 样式不一致——个人中心页导航过宽，logo 和头像间距不同。

### 根因
多个页面的 `<div class="header">` 和内部 `.nav`、`.logo`、`.search-container` 的 CSS 各自独立定义，未统一。

### 修复方案
1. 以 `index.html` 的 header 为标准模板
2. 对比并统一以下页面的 header CSS：
   - `user/profile.html` — 导航过宽
   - `all_questions.html`
   - `one_question.html`
   - `about.html`
   - `judge_result.html`
   - `solutions/detail.html`
   - `solutions/new.html`
   - `user/settings.html`
3. 确保 logo、搜索框、头像的 margin/padding/flex 值一致
4. 头像统一加 `object-fit: cover; border-radius: 50%`

### 验证
- 逐个页面检查 header 外观与 index.html 一致
- 检查 logo(40x40) 和头像(40x40) 的间距

---

## Bug 2: 题解评论数删除后不更新

### 现状
删除某题解下全部评论后，题解列表仍显示原始评论数。

### 根因
`CreateComment` 仅对顶级评论 +1，但 `DeleteComment`（`oj_model.hpp:2141`）降数时：
```cpp
unsigned int dec = 1 + static_cast<unsigned int>(child_count);
```
无论删除的是顶级评论还是回复，都降 `1 + child_count`。删除回复时降了 1，但回复从未升过。

### 修复方案
`oj_model.hpp:2141`：
```cpp
// 仅顶级评论参与 comment_count，回复不计数
if (comment_parent_id == 0) {
    unsigned int dec = 1;
    // ... UPDATE comment_count = GREATEST(comment_count - 1, 0)
}
```
需要先查询被删评论的 `parent_id`，已在 `DeleteComment` 开头有连接可用。

或者更简单：不查询 parent_id，改为只降 1（去掉 child_count），因为：
1. `CreateComment` 对顶级评论升 1，对回复升 0
2. `DeleteComment` 删除顶级评论时，子评论被级联删除，但子评论从未被计数
3. 所以降数 = 1（仅主评论），而不管 child_count

**推荐方案**：改为 `unsigned int dec = 1;`（移除 child_count），仅当被删评论是顶级评论（parent_id == 0 或 IS NULL）时执行 UPDATE。

### 验证
- 创建题解 → 发表 3 条一级评论 → 发表 2 条回复 → 题解列表显示 3 → 逐条删除 3 条一级评论 → 题解列表显示 0

---

## Bug 3: 评论/回复成功提示

### 现状
发表评论、嵌套回复后无任何提示信息。

### 根因
`submitDetailComment`、`submitDetailReply`、`submitReply` 成功后直接清空表单或刷新列表，无 toast。

### 修复方案
创建一个简单的内联 toast 函数，复用项目中已有的 `.avatar-msg` 模式或创建新的：

```javascript
function showToast(msg, isError) {
    var toast = document.getElementById('comment-toast');
    if (!toast) {
        toast = document.createElement('div');
        toast.id = 'comment-toast';
        toast.style.cssText = 'position:fixed;top:20px;left:50%;transform:translateX(-50%);padding:10px 24px;border-radius:8px;font-size:14px;z-index:99999;transition:opacity 0.3s;pointer-events:none;';
        document.body.appendChild(toast);
    }
    toast.textContent = msg;
    toast.style.background = isError ? 'rgba(244,67,54,0.9)' : 'rgba(76,175,80,0.9)';
    toast.style.color = '#fff';
    toast.style.opacity = '1';
    clearTimeout(toast._timer);
    toast._timer = setTimeout(function() { toast.style.opacity = '0'; }, 2500);
}
```

在以下位置调用：
- `one_question.html:submitDetailComment` — 成功后 `showToast('发布成功！')`，失败后 `showToast('发布失败', true)`
- `one_question.html:submitDetailReply` — 同上
- `solutions/detail.html:submitReply` — 同上
- `solutions/detail.html:submitComment` — 同上

### 验证
- 发表评论 → 页面顶部出现绿色 "发布成功!"，2.5 秒后消失
- 发表失败 → 红色错误提示

---

## Bug 4: 发布题解编辑器换为 Editor.md

### 现状
`solutions/new.html` 使用 Vditor 编辑器。

### 修复方案
1. 移除 Vditor 的 CSS 和 JS CDN 引用
2. 引入 Editor.md：
   ```html
   <link rel="stylesheet" href="https://cdn.jsdelivr.net/npm/editor.md@1.5.0/css/editormd.min.css">
   <script src="https://cdn.jsdelivr.net/npm/editor.md@1.5.0/editormd.min.js"></script>
   ```
3. 替换初始化代码：
   ```javascript
   var editor = editormd("vditor-editor", {
       width: "100%",
       height: 500,
       path: "https://cdn.jsdelivr.net/npm/editor.md@1.5.0/lib/",
       theme: "dark",
       previewTheme: "dark",
       editorTheme: "pastel-on-dark",
       markdown: "",
       codeFold: true,
       syncScrolling: "single",
       saveHTMLToTextarea: true,
       searchReplace: true,
       htmlDecode: "style,script,iframe",
       emoji: true,
       taskList: true,
       tocm: true,
       tex: true,
       flowChart: true,
       sequenceDiagram: true,
       placeholder: "请输入题解内容..."
   });
   ```
4. 修改取值逻辑：`editor.getMarkdown()` 替换 `vditor.getValue()`
5. 修改清空逻辑：`editor.setValue('')` 替换 `vditor.setValue('')`
6. 保留 dark 主题 CSS 适配

### 验证
- 打开发布题解页面 → Editor.md 正常加载
- 输入 Markdown → 预览正常
- 提交 → 内容正确保存
- Dark 主题与页面风格一致
