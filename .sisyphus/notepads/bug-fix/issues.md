# F3 Manual QA Review — Issues Found

**Date:** 2026-04-29
**Reviewer:** Sisyphus-Junior (F3 Manual QA)

## VERDICT: APPROVE (with 2 minor non-blocking notes)

## Module-by-Module Evidence

### 1. Comment API Fields — ✅ PASS

**Checked:** `comm.hpp` Comment struct vs `oj_control.hpp` JSON serialization (3 methods: GetComments, GetTopLevelComments, GetCommentReplies)

| Struct field | C++ type | JSON key | JSON cast | Match |
|---|---|---|---|---|
| `parent_id` | `unsigned long long` | `parent_id` | `Json::UInt64` | ✅ |
| `reply_to_user_id` | `int` | `reply_to_user_id` | direct | ✅ |
| `reply_to_user_name` | `std::string` | `reply_to_user_name` | direct | ✅ |
| `like_count` | `unsigned int` | `like_count` | direct | ✅ |
| `favorite_count` | `unsigned int` | `favorite_count` | direct | ✅ |

All 5 fields added consistently across all 3 comment API endpoints. Existing fields unchanged.

### 2. User Stats Accuracy Formula — ✅ PASS

**Backend (`oj_model.hpp:3208-3214`):**
- NEW: `passed_submits` = count of all `is_pass=1` submissions
- FIXED: `accuracy = passed_submits / total_submits` (was incorrectly `passed_questions / total_submits`)
- Rationale: `passed_questions` counts distinct questions (a user passing the same question twice only counts once), which undercounts for accuracy calculation

**Frontend `user/profile.html`:**
- `stat-passed` ← `stats.passed_questions` ✅
- `stat-total` ← `stats.total_submits` ✅
- `stat-accuracy` ← `(accuracy * 100).toFixed(1) + '%'` ✅

**Frontend `spa/pages/profile.html`:**
- `stat-solved` ← `stats.passed_questions` ✅
- `stat-submitted` ← `stats.total_submits` ✅
- No accuracy display (intentional for SPA profile)

### 3. Custom Test Flag — ✅ PASS

**Flow trace:**
```
oj_control.hpp:530  → compile_value["is_custom_test"] = true
  ↓ (JSON via POST /compile_and_run)
runner.hpp:144-152  → parse JSON, set g_is_custom_test = _is_custom BEFORE fork()
  ↓ (parent process retains value through chain)
COP_hanlder.hpp:136 → if (ns_runner::g_is_custom_test) { return stdout/stderr }
custom_flag.cpp      → thread_local bool g_is_custom_test = false (definition)
```

**Include chain:** runner.hpp(18) includes COP_hanlder.hpp(16-18) which declares `extern thread_local bool g_is_custom_test` in `ns_runner`. `custom_flag.cpp` includes COP_hanlder.hpp and defines the variable. CMakeLists.txt adds `custom_flag.cpp` to compile_server. Build passes.

**Thread safety:** Flag correctly reset to false at start of each runner::Execute() call (`_is_custom = false` at line 146), so non-custom requests on the same thread won't leak the flag from previous custom requests.

### 4. Search Keyword Round-Trip — ✅ PASS

**`makePageUrl` (writing):**
- `current.keyword` → `params.set('q', ...)`
- `current.queryMode` → `params.set('query_mode', ...)`

**`parseFiltersFromParams` (reading):**
- `params.get('q')` → `keyword` field
- `params.get('query_mode')` → `queryMode` field

**Server `ParseQuestionQuery` (reading):**
- `req.has_param("q")` → `keyword` variable
- `req.has_param("query_mode")` → `query_mode` variable
- Supports modes: `title`, `id`, `both`

Full round-trip: `keyword` ↔ `q` param, `queryMode` ↔ `query_mode` param. Legacy `title`/`id` params preserved.

### 5. Panel Tab Removal — ✅ PASS

**Removed from `one_question.html`:**
- CSS: `.panel-header`, `.panel-tabs`, `.panel-tab`, `.panel-tab.active`, `.panel-tab:hover` — all removed
- HTML: `<div class="panel-header">` with 3 tab buttons — removed, close button relocated inside panel-body
- JS: `updatePanelTabs()` function — removed
- JS: `panelTabs` variable — removed
- JS: 3 `updatePanelTabs()` call sites (showPanel, hidePanel, runTest) — removed
- JS: panel tab click event listener — removed

**Remaining references:** `currentPanelTab` still used internally for content switching via option buttons (btnTestResult/btnTestCases/btnSubmissions). No dead references to removed elements.

### 6. Build & Conventions — ✅ PASS

- `make -j` succeeds, all 3 targets (compile_server, oj_server, oj_admin) build cleanly
- LSP diagnostics clean on all changed C++ files (custom_flag.cpp LSP error is CMake include-path resolution issue only, not a real build error)

## Minor Notes (Non-Blocking)

1. **`// ADD THIS LINE` comment** at `oj_control.hpp:530` — AI-generated instruction artifact left in code. Should be replaced with a descriptive comment (e.g., `// Mark as custom test to bypass normal judge comparison`).

2. **Redundant `extern` in `runner.hpp:151`** — `extern thread_local bool g_is_custom_test;` is already declared in `COP_hanlder.hpp:17` (which runner.hpp includes). The duplicate is harmless but unnecessary.

3. **Minor: `_is_custom` member variable** in runner.hpp is set but its value is only used to copy to `g_is_custom_test`. Could be simplified to set `g_is_custom_test` directly. No compiler warning generated.
