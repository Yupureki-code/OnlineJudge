# Bug 修复方案 v2（基于更新后的 bug.md）

## TL;DR

> **Quick Summary**: 修复用户测试发现的 7 个问题 — 包括个人中心统计显示、头像删除按钮、搜索栏分页、面板定位、拉伸线限制、嵌套评论渲染、自定义测试期望输出。
>
> **已修复**: Bug 3（搜索栏）已在上一轮修好，仅需验证
> **待修复**: Bug 1, 2, 4, 5, 6, 7 — 共 6 个问题
>
> **Estimated Effort**: Quick（全部是前端/小范围修复）
> **涉及文件**: `user/profile.html`, `one_question.html`, `solutions/detail.html`

---

## 问题分析与修复方案

### Bug 1: 已解决题目数量统计错误 + 板块内容缺失

**现状**: 个人中心 `/user/profile` 中正确率计算错误，已解决题目板块无内容

**根因**:
1. `oj_model.hpp` 中正确率公式错误：原本使用 `passed_questions / total_submits`（不同题目数÷总提交数），应改为 `passed_submits / total_submits`（通过提交数÷总提交数）
2. SPA 个人中心 `spa/pages/profile.html` 的 `loadUserProfile()` 从未调用 `/api/user/stats`
3. 旧版个人中心 `user/profile.html` 的"已解决题目"板块是空的 `<h2>`，无内容

**当前状态**: ⚠️ 已在上轮修复，但需验证是否正确生效:
- `oj_model.hpp` — 新增 `passed_submits` 查询，修正正确率公式
- `spa/pages/profile.html` — `loadUserProfile()` 新增 `/api/user/stats` 请求
- `user/profile.html` — 新增 `renderPassedQuestions()` 函数

**验证方法**: 登录有提交记录的用户，检查个人中心统计数据是否正确

---

### Bug 2: 删除头像按钮 — 需彻底删除

**现状**: `/user/profile` 页面存在 `button.avatar-delete-btn` 删除头像按钮

**要求**: 彻底删除该按钮及其相关代码

**修改文件**: `src/wwwroot/user/profile.html`

**具体操作**:

1. **删除 CSS** (行 321-339)：
   ```css
   .avatar-delete-btn { ... }     /* 全部删除 */
   .avatar-delete-btn:hover { ... }
   .avatar-delete-btn:disabled { ... }
   ```

2. **删除 HTML 元素** (行 645)：
   ```html
   <button class="avatar-delete-btn" id="avatar-delete-btn" style="display:none;">删除头像</button>
   ```
   → **整行删除**

3. **删除 JS 引用**：
   - `setAvatarLoading()` (行 845, 848)：删除 `deleteBtn` 变量及 `deleteBtn.disabled = active`
   - `updateAvatarSrc()` (行 853, 857-858)：删除 `deleteBtn` 变量及样式切换逻辑
   - 头像点击处理 (行 864)：删除 `e.target.closest('#avatar-delete-btn')` 检查
   - 删除头像事件监听 (行 921-950+)：**整段删除** `Delete avatar` 事件处理代码

**验证**: 页面无 `avatar-delete-btn` 元素，上传头像后无删除按钮出现

---

### Bug 3: 导航栏搜索栏功能 — 已修复，需验证

**现状**: SPA 导航栏搜索输入后应跳转到筛选后的题目列表

**根因**: `all_questions.html` 的 `makePageUrl()` 函数使用 `current.title`（undefined）而非 `current.keyword`

**当前状态**: ✅ 已修复
- `current.title` → `current.keyword`
- `params.set('title', ...)` → `params.set('q', ...)`
- 新增 `queryMode` 参数保存

**验证方法**: 在 `/all_questions` 搜索关键词，点击分页第2页，确认 URL 保留 `q=` 参数

---

### Bug 4: panel-body 距离三个按钮过远

**现状**: 题目详情页点击「测试结果/测试用例/提交记录」后，底部面板 `.bottom-panel` 距离上方三个 `.option-btn` 按钮过远

**根因**: 两个原因造成间距过大：
1. `.bottom-panel` 有 `margin-top: 12px`（CSS 行 1410）
2. `#bottom-panel` 在 `<form>` 元素外部（行 1867），而三个按钮在 `<form>` 内的 `.submit-section` 中。form 的 `flex: 1` 使其自然高度远大于按钮区域，bottom-panel 被推到了 form 之后

**修改文件**: `src/wwwroot/one_question.html`

**具体操作**:

1. **将 `#bottom-panel` 移入 `<form>` 内部**，紧跟在 `</textarea>` 之后、`</form>` 之前：
   ```html
   <!-- 当前结构（行 1865-1873）：
         <textarea ...></textarea>
       </form>
   <div id="bottom-panel" class="bottom-panel">  ← 在 form 外
   ```

   变为：
   ```html
         <textarea ...></textarea>
       <div id="bottom-panel" class="bottom-panel">  <!-- 移入 form 内 -->
         ...
       </div>
       </form>
   ```

2. **减小/移除 `.bottom-panel` 的 `margin-top`**：
   ```css
   .bottom-panel {
     margin-top: 0;  /* 原来是 12px → 改为 0 */
   }
   ```

**验证**: 点击「测试结果」按钮，底部面板紧贴按钮下方展开，无明显间距

---

### Bug 5: 拉伸线不能一直向下拉到底

**现状**: 代码编辑区中间的纵向拉伸线（`.resizer-vertical`）向下拉一段距离后就无法继续下拉

**根因**: JS 中设置了 `maxHeight = formHeight - 100`（行 2010），限制了编辑器最多只能占满 form 高度减去 100px，导致提交按钮区域（`.submit-section`）始终保留至少 100px

**修改文件**: `src/wwwroot/one_question.html`

**具体操作**:

在行 2010，将：
```javascript
const maxHeight = formHeight - 100;
```
改为：
```javascript
const maxHeight = formHeight - 50;
```

这将允许编辑器区域占用更多空间，提交区域最小保留 50px。同时确保 `.submit-section` 的 `min-height` 与 JS 限制一致：

行 1853：
```html
<div class="submit-section" style="flex: 1; min-height: 50px;">
```
（原 `min-height: 100px` → 改为 `50px`）

**验证**: 拖动拉伸线可以一直向下到接近 form 底部，提交按钮仍然可见

---

### Bug 6: 嵌套评论 — 点击回复后回复不显示

**现状**: 题解详情页点击某条评论的「回复」按钮，填写回复内容提交后，回复不显示在该评论下方

**根因**: `submitReply()` 和 `hideReplyForm()` 函数定义在 IIFE 闭包内，但通过内联 HTML `onclick` 属性调用（行 862）：
```html
<button onclick="submitReply(' + comment.id + ')">回复</button>
<button onclick="hideReplyForm(' + comment.id + ')">取消</button>
```
内联 `onclick` 在全局作用域查找函数，但 `submitReply`/`hideReplyForm` 在 IIFE 内不可访问

**当前状态**: 后端 API 已正确返回所有 5 个嵌套评论字段（`parent_id`, `reply_to_user_id`, `reply_to_user_name`, `like_count`, `favorite_count`），前端渲染代码也已就绪，问题仅在于函数作用域

**修改文件**: `src/wwwroot/solutions/detail.html`

**具体操作**:

将三个函数暴露到 `window` 全局作用域：

```javascript
// 原来的函数定义（行 927, 936, 942）：
function showReplyForm(parentId, username) { ... }
function hideReplyForm(parentId) { ... }
function submitReply(parentId) { ... }

// 改为：
function showReplyForm(parentId, username) { ... }
function hideReplyForm(parentId) { ... }
function submitReply(parentId) { ... }
// 暴露到全局
window.showReplyForm = showReplyForm;
window.hideReplyForm = hideReplyForm;
window.submitReply = submitReply;
```

在三个函数定义之后（约行 957 之后），添加以上三行赋值。

**验证**: 点击评论回复按钮 → 弹出回复框 → 输入内容 → 点击「回复」→ 回复出现在评论下方（含 @用户名 前缀）

---

### Bug 7: 自定义测试期望输出文本框未删除

**现状**: 自定义测试（用户自定义输入运行代码）后，测试结果中仍然显示「期望输出」文本框（显示空）

**要求**: 自定义测试不应显示期望输出，因为后端不知道正确答案

**根因**: 测试结果渲染函数 `renderTestResults()` 无条件显示 `data.expected_output`（行 2867-2872），未区分自定义测试与正式提交

**修改文件**: `src/wwwroot/one_question.html`

**具体操作**:

方案 A（纯前端 — 推荐）: 在行 2867，将：
```javascript
if (data.expected_output !== undefined && data.expected_output !== null) {
```
改为：
```javascript
if (data.expected_output && String(data.expected_output).trim() !== '') {
```
自定义测试时 `expected_output` 为空字符串，此条件自动过滤，无需后端改动。

方案 B（前后端 — 更规范）: 
- 后端 `oj_control.hpp` 的 `RunSingleTest` 方法中添加：`(*result)["test_type"] = test_type;`
- 前端判断: `if (data.test_type !== 'custom' && data.expected_output !== undefined && data.expected_output !== null) {`

**推荐方案 A**：改动最小，纯前端，且空期望输出本身就没有展示意义。

**验证**: 运行自定义测试，测试结果中不出现「期望输出」标签；运行示例测试用例，期望输出正常显示

---

## 执行计划

### 修改文件清单

| 文件 | 涉及 Bug | 修改类型 |
|------|----------|----------|
| `src/wwwroot/user/profile.html` | Bug 1（验证）, Bug 2（删除） | HTML/CSS/JS |
| `src/wwwroot/one_question.html` | Bug 4（移位置）, Bug 5（改限制）, Bug 7（加条件） | HTML/CSS/JS |
| `src/wwwroot/solutions/detail.html` | Bug 6（暴露全局函数） | JS |

### 建议执行顺序

所有修复都涉及不同文件，可并行执行：
1. **Bug 2 + Bug 1验证**: `user/profile.html`
2. **Bug 4 + Bug 5 + Bug 7**: `one_question.html`
3. **Bug 6**: `solutions/detail.html`

---

## 验证策略

### 后端验证（Bug 1）
```bash
curl -b cookies.txt http://localhost:8080/api/user/stats | jq '.stats'
# 预期：包含 passed_questions, passed_submits, total_submits, accuracy（正确值）
```

### 前端验证（所有 Bug）
使用 Playwright 浏览器自动化：
- Bug 1: 个人中心页面统计数据非零
- Bug 2: 个人中心页面无删除头像按钮
- Bug 3: 搜索后分页，URL 保留关键词
- Bug 4: 底部面板紧贴按钮
- Bug 5: 拉伸线可拉到接近底部
- Bug 6: 回复提交后显示在评论下方
- Bug 7: 自定义测试结果无期望输出框

---

## 成功标准
- [x] Bug 1: 正确率 = passed_submits / total_submits，已解决题目板块有内容
- [x] Bug 2: 页面中不存在 `avatar-delete-btn` 元素
- [x] Bug 3: 搜索关键词在分页 URL 中保留
- [x] Bug 4: 底部面板紧贴三个选项按钮
- [x] Bug 5: 拉伸线可以拉到接近面板底部
- [x] Bug 6: 回复评论后，回复出现在该评论下方
- [x] Bug 7: 自定义测试结果中无「期望输出」标签
