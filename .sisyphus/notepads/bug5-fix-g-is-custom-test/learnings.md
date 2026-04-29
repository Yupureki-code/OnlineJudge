# Bug 5 Fix: g_is_custom_test before fork()

## Root Cause
`g_is_custom_test = _is_custom` was set inside the `pid == 0` (child) block after `fork()`. 
The parent process never sees this value because `fork()` creates separate memory spaces.
The parent later calls `HandlerProgramEnd()` and `HandlerJudge::Execute()` which check 
`g_is_custom_test` → it's always `false` → judge always returns WA for custom tests.

## Fix Applied
Moved JSON parsing and flag assignment BEFORE `fork()` in `runner.hpp`:
1. Lines 142-144: `Json::Value in_value; Json::Reader reader; reader.parse(...)` 
2. Lines 146-149: `_is_custom` detection from `in_value["is_custom_test"]`
3. Lines 151-152: `g_is_custom_test = _is_custom;`

Removed duplicate lines 148-158 from child block (now before fork).
Child still accesses `in_value` via fork's copy-on-write memory inheritance.

## Key Insight
After fork(), child inherits parent's memory (copy-on-write). So `in_value` parsed before fork 
is still accessible in the child. The child reads `in_value["custom_input"]` etc. — not `g_is_custom_test`.
`g_is_custom_test` is only checked in the judge/HandlerProgramEnd path (parent process).
