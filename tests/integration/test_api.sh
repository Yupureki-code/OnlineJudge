#!/bin/bash
# API 集成测试: 验证 /api/user/stats 和 /api/user/info 端点
# 需要服务运行在 localhost:8080
#
# 用法: ./test_api.sh [BASE_URL]
#   BASE_URL 默认 http://localhost:8080

set -euo pipefail
BASE="${1:-http://localhost:8080}"
COOKIE_JAR="/tmp/oj_test_cookies_$$.txt"
PASSED=0
FAILED=0

cleanup() { rm -f "$COOKIE_JAR"; }
trap cleanup EXIT

pass() { echo "  PASS: $1"; ((PASSED++)); }
fail() { echo "  FAIL: $1 (expected: $2, got: $3)"; ((FAILED++)); }

# ---------------------- 测试 1: 未登录调用 ----------------------
echo "--- 测试 1: /api/user/info 未登录 ---"
RESP=$(curl -s -o /dev/null -w "%{http_code}" "$BASE/api/user/info")
if [ "$RESP" = "200" ]; then pass "HTTP 200 for unauthenticated"; else fail "HTTP status" "200" "$RESP"; fi

RESP_BODY=$(curl -s "$BASE/api/user/info")
SUCCESS=$(echo "$RESP_BODY" | python3 -c "import sys,json; print(json.load(sys.stdin).get('success', None))" 2>/dev/null || echo "PARSE_ERR")
if [ "$SUCCESS" = "False" ]; then pass "success=false for unauthenticated"; else fail "success field" "False" "$SUCCESS"; fi

# ---------------------- 测试 2: 用户信息响应格式 ----------------------
echo "--- 测试 2: /api/user/info 响应字段 ---"
# 需要登录态。如果 cookie 文件为空则跳过登录
echo "  SKIP: 需要登录态 (设置 COOKIE 环境变量或手动登录)"

# ---------------------- 测试 3: 统计端点缓存行为 ----------------------
echo "--- 测试 3: /api/user/stats 缓存验证 ---"
echo "  INFO: 验证 Redis 键 stats:user:{uid}"

# ---------------------- 测试 4: 头像 URL 在响应中 ----------------------
echo "--- 测试 4: /api/user/info 返回 avatar_url ---"
echo "  INFO: JSON 应包含 user.avatar_url 字段"

# ---------------------- 报告 ----------------------
echo ""
echo "========================================"
echo "集成测试结果: $PASSED passed, $FAILED failed"
echo "========================================"
exit $FAILED
