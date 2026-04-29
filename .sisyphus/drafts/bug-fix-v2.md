# Draft: Bug Fix Plan v2 (updated bug.md)

## Requirements (from updated bug.md)
1. **Bug 1**: 已解决题目数量统计错误 + 已解决题目板块未显示
2. **Bug 2**: 删除头像按钮 — 彻底删除HTML元素 (NEW)
3. **Bug 3**: 导航栏搜索栏功能 — (可能已修复，需验证)
4. **Bug 4**: panel-body位置不对 — 让底部面板紧贴三个选项按钮 (CHANGED)
5. **Bug 5**: 拉伸线不能一直下拉到底 (NEW)
6. **Bug 6**: 嵌套评论 — 点击回复后不显示 (后端已修，前端渲染问题)
7. **Bug 7**: 自定义测试期望输出文本框 — 删除该文本框 (SIMPLIFIED)

## User Decisions
- Bug 2: 彻底删除HTML元素（button.avatar-delete-btn + JS）
- Bug 4: panel-body紧贴「测试结果」「测试用例」「提交记录」三个按钮
- Bug 6: 点击回复按钮后回复未显示在下方的渲染问题

## Scope Boundaries
- IN: All 7 bugs from updated bug.md
- EXCLUDE: Any new features, refactoring, unrelated fixes
