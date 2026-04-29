# Bug Fix Plan: 5 Bugs from bug.md

## TL;DR

> **Quick Summary**: Fix 5 bugs found in post-implementation testing: user stats display, search bar pagination, redundant panel header, missing comment API fields, and custom test WA error.
>
> **Deliverables**:
> - Fixed user profile stats (SPA + old page)
> - Working search bar with pagination keyword preservation
> - Removed redundant panel-tabs on question detail page
> - Comment API now returns all 5 nested-comment fields
> - Custom test returns raw output instead of WA
>
> **Estimated Effort**: Medium
> **Parallel Execution**: YES — 2 waves (3 backend + 3 frontend tasks)
> **Critical Path**: Bug 4 (API contract) → Bug 1 (consumes API) → Bug 2,3 (independent frontend)

---

## Context

### Original Request
User tested the OnlineJudge platform after a previous implementation and documented 5 bugs in `bug.md`. Each bug has: current behavior, desired behavior, and a verification suggestion.

### Interview Summary
**Key Discussions**:
- Test strategy: No automated tests — Agent-Executed QA Scenarios (curl for API, Playwright for browser)
- Fix scope: Surgical line-level fixes only, no refactoring, no new endpoints
- Bug 4 (comment API fields) is foundational — must be fixed first so frontend fixes (1,2,3) can verify against correct API responses

**Research Findings**:
- Bug 1 root cause: SPA profile page `loadUserProfile()` never calls `/api/user/stats`; old profile has empty "已解决题目" section. Backend data is correct.
- Bug 2 root cause: `makePageUrl()` in `all_questions.html:1278` checks `current.title` (undefined) instead of `current.keyword`
- Bug 3 root cause: No "panel-header" class exists; `.panel-tabs` div (`one_question.html:1894`) is the redundant tab bar
- Bug 4 root cause: Controller JSON serialization missing 5 fields in `GetTopLevelComments` and `GetCommentReplies` — Model already queries correctly
- Bug 5 root cause: `g_is_custom_test` flag set inside `fork()` child block (line 151-158) — parent never sees it; parent's flag is always `false`

### Metis Review
**Identified Gaps** (addressed):
- Bug 1 old-profile scope: Use count display + filter `recent_submits` for `is_pass=true` list (avoids new API endpoint)
- Bug 3 fix approach: Remove `.panel-tabs` entirely (user's explicit request: "删除panel-header")
- Bug 5 fix strategy: Move `in_json` parsing + `g_is_custom_test` assignment BEFORE `fork()`, remove lines 157-158 from child
- Bug 5 edge cases: Also fix error paths (lines 300,307,313) — they call `HandlerProgramEnd` which also reads `g_is_custom_test`
- Fix order: Bug 4 (API) → Bug 1 (consumes API) → Bug 2,3 (independent frontend) → Bug 5 (separate process)

---

## Work Objectives

### Core Objective
Fix all 5 bugs documented in `bug.md`, with each fix verified via Agent-Executed QA Scenarios.

### Concrete Deliverables
- `src/wwwroot/spa/pages/profile.html` — SPA profile loads and displays stats
- `src/wwwroot/user/profile.html` — Old profile populates "已解决题目" section
- `src/wwwroot/all_questions.html` — Search keyword preserved across pagination
- `src/wwwroot/one_question.html` — Redundant `.panel-tabs` removed
- `src/oj_server/include/oj_control.hpp` — Comment API JSON includes 5 missing fields
- `src/compile_server/include/runner.hpp` — Custom test flag visible to parent process

### Definition of Done
- [ ] Bug 1: Login as test user → SPA profile shows non-zero solved count and total submits
- [ ] Bug 1: Login → old profile "已解决题目" section shows passed question list
- [ ] Bug 2: Search "two sum" → paginate to page 2 → URL retains `q=` parameter, results stay filtered
- [ ] Bug 3: Question detail page → bottom panel visible only via `.info-option` buttons, no `.panel-tabs`
- [ ] Bug 4: `curl /api/solutions/{id}/comments` → each comment has `parent_id`, `reply_to_user_id`, `reply_to_user_name`, `like_count`, `favorite_count`
- [ ] Bug 5: POST custom test → response status is `OK` with `stdout`/`stderr`, never `WA`

### Must Have
- Surgical minimal changes — no refactoring, no restructuring
- Bug 4 must be fixed first (foundational API change)
- All 5 bugs fixed and verified

### Must NOT Have (Guardrails)
- No new API endpoints
- No new files
- No DB schema changes
- No test framework, no CI setup
- No refactoring (DRY helper functions, pagination module extraction, process architecture changes)
- No fixing ancillary issues (SQL injection in runner.hpp, empty catch blocks, logging emails)
- No redesign of profile UI, panel layout, or search UX

---

## Verification Strategy

> **ZERO HUMAN INTERVENTION** — ALL verification is agent-executed.

### Test Decision
- **Infrastructure exists**: NO
- **Automated tests**: None
- **Framework**: None

### QA Policy
Every task includes Agent-Executed QA Scenarios:
- **API/Backend**: Use Bash (curl) — Send requests, assert status + response fields
- **Frontend/UI**: Use Playwright — Navigate, interact, assert DOM, screenshot
- **Compile Server**: Use Bash (curl) — POST compile_and_run, assert response fields

Evidence saved to `.sisyphus/evidence/task-{N}-{scenario-slug}.{ext}`.

---

## Execution Strategy

### Parallel Execution Waves

```
Wave 1 (Start Immediately — Backend fixes, MAX PARALLEL):
├── Task 1: Bug 4 — Add 5 missing fields to comment API JSON [quick]
├── Task 2: Bug 5 — Fix custom test g_is_custom_test flag scope [deep]
└── Task 3: Bug 5 — Move in_json parsing before fork [deep]  (NOTE: Task 2+3 are same file, sequential)

Wave 2 (After Wave 1 — Frontend fixes, MAX PARALLEL):
├── Task 4: Bug 1 — Fix SPA profile page stats loading [quick]
├── Task 5: Bug 1 — Fix old profile page passed questions [quick]
├── Task 6: Bug 2 — Fix search keyword in pagination URL [quick]
└── Task 7: Bug 3 — Remove redundant panel-tabs [quick]
```

**Critical Path**: Task 1 (Bug4 API fields) → Task 4 (SPA stats) / Task 5 (old profile) — frontend needs API changes first
**Parallel Speedup**: ~50% faster than sequential

---

## TODOs

- [x] 1. **Bug 4** — Add 5 missing fields to comment API JSON responses

  **What to do**:
  - In `oj_control.hpp`, for ALL 3 comment retrieval methods, add these 5 fields to the JSON response:
    - `GetComments()` (line ~1231): Add `parent_id`, `reply_to_user_id`, `reply_to_user_name`, `like_count`, `favorite_count`
    - `GetTopLevelComments()` (line ~1285): Same 5 fields (note: `reply_count` already included)
    - `GetCommentReplies()` (line ~1346): Same 5 fields
  - Field source: `Comment` struct (`comm.hpp:158-172`) already has all fields; Model already queries and parses them
  - Add after `item["updated_at"]` in each loop:
    ```cpp
    item["parent_id"] = Json::UInt64(c.parent_id);
    item["reply_to_user_id"] = c.reply_to_user_id;
    item["reply_to_user_name"] = c.reply_to_user_name;
    item["like_count"] = c.like_count;
    item["favorite_count"] = c.favorite_count;
    ```

  **Must NOT do**:
  - Must NOT change Model (`oj_model.hpp`) — already correct
  - Must NOT change frontend (`detail.html`) — already expects these fields
  - Must NOT refactor the 3 methods into a shared helper

  **Recommended Agent Profile**:
  - **Category**: `quick`
    - Reason: Simple surgical fix — copy the same 5 lines into 3 locations
  - **Skills**: `[]`
  - **Skills Evaluated but Omitted**: None

  **Parallelization**:
  - **Can Run In Parallel**: YES (with Task 2-3 are same file in runner.hpp — independent)
  - **Parallel Group**: Wave 1
  - **Blocks**: Task 4 (SPA profile), Task 5 (old profile) — frontend needs API fields
  - **Blocked By**: None

  **References**:
  - `src/oj_server/include/oj_control.hpp:1231-1252` — `GetComments()` JSON construction (missing 5 fields)
  - `src/oj_server/include/oj_control.hpp:1282-1316` — `GetTopLevelComments()` JSON construction (missing 5 fields)
  - `src/oj_server/include/oj_control.hpp:1346-1368` — `GetCommentReplies()` JSON construction (missing 5 fields)
  - `src/comm/comm.hpp:158-172` — `Comment` struct — verify field names match

  **Acceptance Criteria**:
  - [ ] All 3 controller functions include `parent_id`, `reply_to_user_id`, `reply_to_user_name`, `like_count`, `favorite_count` in JSON

  **QA Scenarios**:

  ```
  Scenario: GET /api/solutions/{id}/comments returns all 5 fields per comment
    Tool: Bash (curl)
    Preconditions: oj_server running at localhost:8080, at least 1 solution with comments exists
    Steps:
      1. curl -s 'http://localhost:8080/api/solutions/1/comments?page=1&size=10'
      2. Parse JSON with jq: jq '.comments[0] | keys'
    Expected Result: JSON comment objects contain keys: parent_id, reply_to_user_id, reply_to_user_name, like_count, favorite_count
    Failure Indicators: Missing any of the 5 keys; comment listing returns error
    Evidence: .sisyphus/evidence/task-1-comment-fields.txt

  Scenario: GET /api/comments/{id}/replies returns all 5 fields per reply
    Tool: Bash (curl)
    Preconditions: oj_server running, at least 1 comment has nested replies
    Steps:
      1. Get top-level comment ID from first scenario
      2. curl -s "http://localhost:8080/api/comments/1/replies?page=1&size=10"
      3. Parse JSON: jq '.comments[0] | keys'
    Expected Result: Each reply object contains all 5 fields
    Failure Indicators: Missing fields in reply objects
    Evidence: .sisyphus/evidence/task-1-reply-fields.txt
  ```

  **Commit**: YES
  - Message: `fix(api): add missing comment fields to API response`
  - Files: `src/oj_server/include/oj_control.hpp`

- [x] 2. **Bug 5** — Move g_is_custom_test flag assignment before fork()

  **What to do**:
  - In `src/compile_server/include/runner.hpp`, `HandlerRunner::Execute()`:
    1. Move `in_json` parsing from child block (lines 148-150) to BEFORE `fork()` (before line 140):
       ```cpp
       Json::Value in_value;
       Json::Reader reader;
       reader.parse(in_json, in_value);
       ```
    2. Move `_is_custom` detection from child (lines 151-155) to before fork:
       ```cpp
       _is_custom = false;
       if (in_value.isMember("is_custom_test") && in_value["is_custom_test"].asBool()) {
           _is_custom = true;
       }
       ```
    3. Set `g_is_custom_test` BEFORE fork (so parent sees it):
       ```cpp
       g_is_custom_test = _is_custom;
       ```
    4. In the child block (pid==0): REMOVE lines 148-158 (duplicate parsing + flag setting — now handled before fork)
    5. Verify the child's custom test logic (lines 164-172) still works: it checks `in_value["is_custom_test"]` and `in_value["custom_input"]` independently of `g_is_custom_test`

  **Root cause**: `g_is_custom_test` was set inside child (pid==0) after `fork()`. Parent's copy stays `false`. Parent later calls `HandlerProgramEnd()` and `HandlerJudge::Execute()` which check `g_is_custom_test` → never detects custom test → WA is returned.

  **Must NOT do**:
  - Must NOT change `HandlerProgramEnd()` in `COP_hanlder.hpp` — the guard at line 136 is correct, it just never fired
  - Must NOT change `HandlerJudge::Execute()` logic — same reason
  - Must NOT change the child's custom test flow (lines 164-287)
  - Must NOT remove the `_is_custom` member or `g_is_custom_test` extern — both still needed

  **Recommended Agent Profile**:
  - **Category**: `deep`
    - Reason: Delicate fork/process fix — must understand process memory isolation and verify both parent and child paths work correctly
  - **Skills**: `[]`
  - **Skills Evaluated but Omitted**: None

  **Parallelization**:
  - **Can Run In Parallel**: NO (shares file with Task 3; sequential with it)
  - **Parallel Group**: Wave 1 (with Task 1, parallel to Task 1)
  - **Blocks**: None (compile_server is independent process)
  - **Blocked By**: None

  **References**:
  - `src/compile_server/include/runner.hpp:134-327` — Full `HandlerRunner::Execute()` method
  - `src/compile_server/include/runner.hpp:140` — `fork()` call line
  - `src/compile_server/include/runner.hpp:146-158` — Child block with current flag setting (to be moved)
  - `src/compile_server/include/runner.hpp:289-321` — Parent process after `waitpid()` (reads `g_is_custom_test`)
  - `src/compile_server/include/COP_hanlder.hpp:135-151` — `HandlerProgramEnd` guard that checks `g_is_custom_test`
  - `src/compile_server/src/custom_flag.cpp:4-5` — `thread_local bool g_is_custom_test` definition

  **Acceptance Criteria**:
  - [ ] `g_is_custom_test` set before `fork()` — parent process sees correct value
  - [ ] Child custom test logic (lines 164-172) still works — checks `in_value`, not `g_is_custom_test`
  - [ ] Removed duplicate lines 148-158 from child block
  - [ ] Both error path (lines 300/307/313) AND success path (line 320→judge) use correct flag value

  **QA Scenarios**:

  ```
  Scenario: Custom test returns OK with stdout/stderr
    Tool: Bash (curl)
    Preconditions: compile_server running on port 8081, oj_server running on 8080, valid question exists
    Steps:
      1. POST to compile_server /compile_and_run with:
         {"is_custom_test": true, "custom_input": "1 2", "code": "#include <iostream>\nint main() { int a,b; std::cin>>a>>b; std::cout<<a+b; return 0; }", "cpu_limit": 1, "mem_limit": 128}
      2. Parse JSON response
    Expected Result: Response has "status": "OK" (NOT "WA", "AC", etc.), and "stdout" field contains "3"
    Failure Indicators: status is "WA" or "AC"; stdout not present; expected_output field present
    Evidence: .sisyphus/evidence/task-2-custom-test-ok.json

  Scenario: Normal submission still returns judge result (AC/WA)
    Tool: Bash (curl)
    Preconditions: Same as above, plus a question with test cases where expected output is "3"
    Steps:
      1. POST to compile_server /compile_and_run WITHOUT is_custom_test flag, WITH question_id
      2. Parse JSON response
    Expected Result: Response has status like "AC" (if output matches) or "WA"
    Failure Indicators: Status always "OK" (regression — custom test flag leaking)
    Evidence: .sisyphus/evidence/task-2-normal-submit.json
  ```

  **Commit**: NO (combined with Task 3)

- [x] 3. **Bug 5** — Verify child custom test path and clean up

  **What to do**:
  - After Task 2, verify the child block still correctly handles custom tests:
  - Lines 164-168: Check `has_custom_input` from `in_value["custom_input"]` → creates a single Test with `t.out = custom_output`
  - Lines 173-183: Custom test branch writes custom_input to stdin, gets exactly 1 test case
  - Lines 233-237: Writes `test.out` (custom_output) to `.ans` file
  - Verify no other code in child block references `_is_custom` or `g_is_custom_test` in a way that breaks
  - Remove the now-redundant `_is_custom` parsing from child (lines 151-155 already moved to Task 2 pre-fork)
  - Remove the `g_is_custom_test = _is_custom` assignment from child (line 158 already handled pre-fork)

  **Must NOT do**:
  - Must NOT remove `_is_custom` member variable — still used (or will be deprecated)
  - Must NOT change the child's `exit(0)` or process management logic

  **Recommended Agent Profile**:
  - **Category**: `deep`
    - Reason: Verification task following the fork fix — must trace all code paths
  - **Skills**: `[]`

  **Parallelization**:
  - **Can Run In Parallel**: NO (sequential after Task 2, same file)
  - **Parallel Group**: Wave 1
  - **Blocks**: None
  - **Blocked By**: Task 2

  **References**:
  - `src/compile_server/include/runner.hpp:164-183` — Child custom test branch
  - `src/compile_server/include/runner.hpp:233-237` — `.ans` file write for custom test
  - `src/compile_server/include/runner.hpp:248-287` — Inner child fork for user code execution

  **Acceptance Criteria**:
  - [ ] Child block compiles successfully after removing lines 148-158
  - [ ] Custom test with user input → child writes custom_input to stdin correctly
  - [ ] Normal submission (not custom) → child queries test cases from DB correctly

  **QA Scenarios**:

  ```
  Scenario: Custom test with edge case: empty output
    Tool: Bash (curl)
    Preconditions: compile_server running on port 8081
    Steps:
      1. POST custom test with code that produces no output (e.g., `int main(){return 0;}`)
      2. Parse response
    Expected Result: status "OK", stdout is empty or ""
    Failure Indicators: WA, runtime error, or other judge status
    Evidence: .sisyphus/evidence/task-3-empty-output.json

  Scenario: Custom test with signal (e.g., null pointer dereference)
    Tool: Bash (curl)
    Preconditions: compile_server running
    Steps:
      1. POST custom test with code that dereferences null pointer
      2. Parse response
    Expected Result: stderr contains signal info (not judge result), status NOT "WA"
    Failure Indicators: "WA" status in response
    Evidence: .sisyphus/evidence/task-3-signal.txt
  ```

  **Commit**: NO (combined with Task 2)
  - Message: `fix(runner): move custom test flag before fork`
  - Files: `src/compile_server/include/runner.hpp`

---
- [x] 4. **Bug 1** — Fix SPA profile page to load user stats

  **What to do**:
  - In `src/wwwroot/spa/pages/profile.html`, modify `loadUserProfile()` function:
    1. After the existing `/api/user/info` fetch (line 168), add a second fetch to `/api/user/stats`
    2. Parse the stats response: `data.stats.passed_questions` → `#stat-solved`, `data.stats.total_submits` → `#stat-submitted`
    3. Follow the same pattern as old profile page (`user/profile.html:746-759`):
       ```javascript
       const statsResp = await fetch('/api/user/stats', { credentials: 'include' });
       const statsData = await statsResp.json();
       if (statsData && statsData.success && statsData.stats) {
           document.getElementById('stat-solved').textContent = statsData.stats.passed_questions || 0;
           document.getElementById('stat-submitted').textContent = statsData.stats.total_submits || 0;
       }
       ```
    4. Optionally add accuracy display if the stat element exists

  **Must NOT do**:
  - Must NOT create a new API endpoint — `/api/user/stats` already exists and works
  - Must NOT modify `oj_control.hpp` or `oj_model.hpp` — backend data is correct
  - Must NOT redesign the profile page layout

  **Recommended Agent Profile**:
  - **Category**: `quick`
    - Reason: Simple frontend-only fetch addition following existing pattern
  - **Skills**: `[]`

  **Parallelization**:
  - **Can Run In Parallel**: YES (with Tasks 5, 6, 7 — all different files)
  - **Parallel Group**: Wave 2
  - **Blocks**: None
  - **Blocked By**: Task 1 (Bug 4) — needs correct API response shape

  **References**:
  - `src/wwwroot/spa/pages/profile.html:150-195` — `loadUserProfile()` function to modify
  - `src/wwwroot/spa/pages/profile.html:117-126` — Stat display elements (#stat-solved, #stat-submitted)
  - `src/wwwroot/user/profile.html:746-759` — Working example of same pattern (copy this structure)

  **Acceptance Criteria**:
  - [ ] `loadUserProfile()` calls both `/api/user/info` AND `/api/user/stats`
  - [ ] `#stat-solved` shows actual count (not "0")
  - [ ] `#stat-submitted` shows actual count (not "0")

  **QA Scenarios**:

  ```
  Scenario: SPA profile page displays correct stats
    Tool: Playwright
    Preconditions: oj_server running at localhost:8080, test user logged in (uid=3), user has at least 1 submission
    Steps:
      1. Navigate to localhost:8080/spa/app.html#profile
      2. Wait for network idle (both /api/user/info and /api/user/stats responses received)
      3. Read textContent of element #stat-solved
      4. Read textContent of element #stat-submitted
    Expected Result: #stat-solved textContent is non-zero integer (>0); #stat-submitted textContent is non-zero integer (>0)
    Failure Indicators: Either stat shows "0"; network shows only 1 fetch (missing /api/user/stats)
    Evidence: .sisyphus/evidence/task-4-profile-stats.png (screenshot)
  ```

  **Commit**: YES
  - Message: `fix(spa): load user stats on profile page`
  - Files: `src/wwwroot/spa/pages/profile.html`

- [x] 5. **Bug 1** — Fix old profile page "已解决题目" section

  **What to do**:
  - In `src/wwwroot/user/profile.html`:
    1. Remove the empty "已解决题目" heading (lines 658-660) or add content below it
    2. Add a JS function that filters `data.stats.recent_submits` for entries where `is_pass === true`
    3. Render the filtered list as a simple list of question titles with pass status
    4. Since the backend has no dedicated "passed questions" endpoint and `recent_submits` only has last 20, add a note that this shows recent passes
    - OR: Simply add a call to display the `passed_questions` count in the section as a quick fix:
      ```html
      <div class="section">
        <h2 class="section-title">已解决题目</h2>
        <p>共通过 <strong id="stat-passed-count">0</strong> 道题目</p>
      </div>
      ```
      And update `#stat-passed-count` in `loadUserStats()`.

  **Decision**: Use simple count display approach (matching the stat bar above). Filter `recent_submits` for `is_pass === true` and show the top 5 as clickable links.

  **Must NOT do**:
  - Must NOT add a new backend API endpoint for passed questions list
  - Must NOT leave the empty section

  **Recommended Agent Profile**:
  - **Category**: `quick`
    - Reason: Frontend HTML/JS addition following existing patterns
  - **Skills**: `[]`

  **Parallelization**:
  - **Can Run In Parallel**: YES (with Tasks 4, 6, 7)
  - **Parallel Group**: Wave 2
  - **Blocks**: None
  - **Blocked By**: Task 1 (Bug 4)

  **References**:
  - `src/wwwroot/user/profile.html:658-660` — Empty "已解决题目" section to fix
  - `src/wwwroot/user/profile.html:746-759` — Existing `loadUserStats()` to extend
  - `src/wwwroot/user/profile.html:755-757` — `renderRecentSubmits()` — data source for filtering

  **Acceptance Criteria**:
  - [ ] "已解决题目" section has non-empty content
  - [ ] Clicking passed question titles navigates to that question's detail page

  **QA Scenarios**:

  ```
  Scenario: Old profile shows passed questions
    Tool: Playwright
    Preconditions: oj_server running, test user logged in, user has passed at least 1 question
    Steps:
      1. Navigate to localhost:8080/user/profile
      2. Wait for page load and /api/user/stats response
      3. Check "已解决题目" section for content
      4. Click first passed question title
    Expected Result: Section shows passed question titles; clicking navigates to /questions/{id}
    Failure Indicators: Section is empty; clicking does nothing; 500 error on page load
    Evidence: .sisyphus/evidence/task-5-passed-questions.png (screenshot)
  ```

  **Commit**: YES
  - Message: `fix(profile): populate passed questions section`
  - Files: `src/wwwroot/user/profile.html`

- [x] 6. **Bug 2** — Fix search keyword in pagination URLs

  **What to do**:
  - In `src/wwwroot/all_questions.html`, `makePageUrl()` function (lines 1271-1282):
    1. Change line 1278: `current.title` → `current.keyword`
    2. Change line 1279: `params.set('title', current.title.trim())` → `params.set('q', current.keyword.trim())`
    3. Also add `queryMode` preservation to pagination URLs:
       ```javascript
       if (current.queryMode) {
           params.set('query_mode', current.queryMode);
       }
       ```
  - The static template pagination links (lines 1007-1016) don't need changes — they're overridden by `updatePagination()` which calls `makePageUrl()`

  **Root cause**: `getCurrentFilters()` returns `{keyword: ..., queryMode: ...}` but `makePageUrl()` checked `current.title` (undefined property, always false)

  **Must NOT do**:
  - Must NOT change `parseFiltersFromParams()` — already correct, handles both `q` and `title` params
  - Must NOT change `buildAllQuestionsUrl()` — already correct
  - Must NOT restructure pagination

  **Recommended Agent Profile**:
  - **Category**: `quick`
    - Reason: 2-line JS fix in a single file
  - **Skills**: `[]`

  **Parallelization**:
  - **Can Run In Parallel**: YES (with Tasks 4, 5, 7)
  - **Parallel Group**: Wave 2
  - **Blocks**: None
  - **Blocked By**: None (frontend-only)

  **References**:
  - `src/wwwroot/all_questions.html:1271-1282` — `makePageUrl()` function to fix
  - `src/wwwroot/all_questions.html:1068-1083` — `parseFiltersFromParams()` — returns `keyword`, not `title`
  - `src/wwwroot/all_questions.html:1085-1099` — `buildAllQuestionsUrl()` — correct reference for URL param names

  **Acceptance Criteria**:
  - [ ] Search "two sum" on all_questions → URL shows `?q=two+sum`
  - [ ] Click page 2 → URL shows `?page=2&q=two+sum` (keyword preserved)
  - [ ] Results on page 2 are filtered by the search keyword

  **QA Scenarios**:

  ```
  Scenario: Search keyword preserved across pagination
    Tool: Playwright
    Preconditions: oj_server running at localhost:8080, test user logged in
    Steps:
      1. Navigate to localhost:8080/all_questions
      2. Type "two sum" in input.search-box
      3. Press Enter
      4. Wait for results to load
      5. Click page "2" link in pagination
      6. Check URL for q= parameter
      7. Check that results on page 2 match the search filter
    Expected Result: URL contains "q=two+sum"; results are filtered (only questions whose title contains "two sum")
    Failure Indicators: URL missing q parameter; unfiltered results on page 2
    Evidence: .sisyphus/evidence/task-6-pagination-search.png (screenshot)
  ```

  **Commit**: YES
  - Message: `fix(search): preserve keyword in pagination URL`
  - Files: `src/wwwroot/all_questions.html`

- [x] 7. **Bug 3** — Remove redundant panel-tabs from question detail page

  **What to do**:
  - In `src/wwwroot/one_question.html`:
    1. Remove the `.panel-tabs` HTML block (lines 1894-1899):
       ```html
       <div class="panel-tabs">   <!-- DELETE -->
           ...                     <!-- DELETE all 5 child elements -->
       </div>                      <!-- DELETE -->
       ```
    2. Remove the `updatePanelTabs()` function (lines 2661-2666) and its call sites at lines 2650, 2691, 2835
    3. Optionally remove the unused CSS for `.panel-tabs`, `.panel-tab`, `.panel-tab.active`, `.panel-tab:hover`, `.panel-close` (lines 1425-1478)
    4. Verify `.option-btn` buttons (lines 1881-1883) still open the panel correctly via `showPanel()`
    5. Verify close button is still accessible — the close button was inside `.panel-tabs`. Replace it either:
       - Move the close button (`&times;`) to the `.panel-body` header, OR
       - Add inline close button to panel-body's top
       - Simplest: add `<button class="panel-close" onclick="hidePanel()" style="position:absolute;top:8px;right:8px;">&times;</button>` at the top of `.panel-body`

  **Must NOT do**:
  - Must NOT remove `.info-option` buttons (lines 1881-1883) — they're the primary panel triggers
  - Must NOT break the `showPanel()/hidePanel()` toggle logic
  - Must NOT remove `#bottom-panel` or `.panel-body` elements

  **Recommended Agent Profile**:
  - **Category**: `quick`
    - Reason: HTML/CSS removal + minor JS cleanup in a single file
  - **Skills**: `[]`

  **Parallelization**:
  - **Can Run In Parallel**: YES (with Tasks 4, 5, 6)
  - **Parallel Group**: Wave 2
  - **Blocks**: None
  - **Blocked By**: None

  **References**:
  - `src/wwwroot/one_question.html:1893-1903` — Full `.bottom-panel` HTML structure
  - `src/wwwroot/one_question.html:1894-1899` — `.panel-tabs` block to remove
  - `src/wwwroot/one_question.html:2661-2666` — `updatePanelTabs()` function to remove
  - `src/wwwroot/one_question.html:2643-2658` — `showPanel()` to verify still works

  **Acceptance Criteria**:
  - [ ] No `.panel-tabs` element in the HTML
  - [ ] Clicking `.option-btn` (测试结果/测试用例/提交记录) opens `.bottom-panel` with correct content
  - [ ] Close button (×) visible and functional
  - [ ] Panel content loads correctly for each tab

  **QA Scenarios**:

  ```
  Scenario: Panel opens via option-btn after panel-tabs removal
    Tool: Playwright
    Preconditions: oj_server running, test user logged in, viewing a question detail page
    Steps:
      1. Navigate to question detail page (e.g., /questions/1)
      2. Click button #btn-test-cases ("测试用例")
      3. Wait for panel to expand
      4. Check that #bottom-panel has class "visible"
      5. Check that NO element with class "panel-tabs" exists in DOM
      6. Check panel content loaded (contains test case data)
    Expected Result: Panel opens, shows test case content, no panel-tabs bar
    Failure Indicators: Panel doesn't open; panel-tabs still visible; content not loading
    Evidence: .sisyphus/evidence/task-7-panel-open.png (screenshot)

  Scenario: Close button still works after panel-tabs removal
    Tool: Playwright
    Preconditions: Panel is open from scenario above
    Steps:
      1. Click the close button (×)
      2. Check #bottom-panel does NOT have class "visible"
    Expected Result: Panel closes
    Failure Indicators: Panel stays open; close button not found
    Evidence: .sisyphus/evidence/task-7-panel-closed.png (screenshot)
  ```

  **Commit**: YES
  - Message: `fix(ui): remove redundant panel-tabs`
  - Files: `src/wwwroot/one_question.html`

---
- [x] F1. **Plan Compliance Audit** — `oracle`: Verify all 5 "Must Have" fixed, 8 "Must NOT Have" not violated
- [x] F2. **Code Quality Review** — `unspecified-high`: Run `tsc --noEmit` (if applicable), check for regressions
- [x] F3. **Real Manual QA** — `unspecified-high` + `playwright`: Execute ALL QA scenarios from every task
- [x] F4. **Scope Fidelity Check** — `deep`: Verify each fix is minimal (lines changed, no creep)

---

## Commit Strategy

- **1**: `fix(api): add missing comment fields to API response` — `src/oj_server/include/oj_control.hpp`
- **2-3**: `fix(runner): move custom test flag before fork` — `src/compile_server/include/runner.hpp`
- **4**: `fix(spa): load user stats on profile page` — `src/wwwroot/spa/pages/profile.html`
- **5**: `fix(profile): populate passed questions section` — `src/wwwroot/user/profile.html`
- **6**: `fix(search): preserve keyword in pagination URL` — `src/wwwroot/all_questions.html`
- **7**: `fix(ui): remove redundant panel-tabs` — `src/wwwroot/one_question.html`

---

## Success Criteria

### Verification Commands
```bash
# Bug 4: Comment API returns all 5 fields
curl -s http://localhost:8080/api/comments/1/replies | jq '.comments[0] | keys'
# Expected: contains parent_id, reply_to_user_id, reply_to_user_name, like_count, favorite_count

# Bug 5: Custom test returns OK not WA
curl -s -X POST http://localhost:8081/compile_and_run -d '{"is_custom_test":true,...}' | jq '.status'
# Expected: "OK"
```

### Final Checklist
- [ ] All 5 bugs fixed per Definition of Done
- [ ] All "Must NOT Have" guardrails respected
- [ ] All QA scenarios pass
- [ ] No new files, no DB changes, no refactoring
