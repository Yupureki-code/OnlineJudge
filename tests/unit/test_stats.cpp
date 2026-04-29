/**
 * 用户统计逻辑单元测试
 * 测试 accuracy 计算公式和缓存条件，无需 MySQL/Redis 即可运行
 *
 * 构建: g++ -std=c++17 -o test_stats test_stats.cpp && ./test_stats
 */
#include <cassert>
#include <cstdio>
#include <cmath>
#include <string>

// 计数器
static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name)                                                       \
    do {                                                                 \
        printf("  TEST: %s ... ", name);                                 \
    } while(0)

#define PASS()                                                           \
    do {                                                                 \
        printf("PASS\n");                                                \
        ++tests_passed;                                                  \
    } while(0)

#define FAIL(fmt, ...)                                                   \
    do {                                                                 \
        printf("FAIL: " fmt "\n", ##__VA_ARGS__);                        \
        ++tests_failed;                                                  \
    } while(0)

#define ASSERT_TRUE(cond, fmt, ...)                                      \
    do {                                                                 \
        if (!(cond)) {                                                   \
            FAIL(fmt, ##__VA_ARGS__);                                    \
            return;                                                      \
        }                                                                \
    } while(0)

#define ASSERT_EQ(a, b, fmt, ...)                                        \
    do {                                                                 \
        if ((a) != (b)) {                                                \
            FAIL(fmt " (expected=%d, actual=%d)", ##__VA_ARGS__, (int)(b), (int)(a)); \
            return;                                                      \
        }                                                                \
    } while(0)

#define ASSERT_FLOAT_EQ(a, b, eps, fmt, ...)                             \
    do {                                                                 \
        if (std::fabs((a) - (b)) > (eps)) {                              \
            FAIL(fmt " (expected=%.6f, actual=%.6f)", ##__VA_ARGS__, (b), (a)); \
            return;                                                      \
        }                                                                \
    } while(0)

// ============================================================
// 被测逻辑: accuracy = total_submits > 0 ? passed / total : 0
// (从 oj_model.hpp:3214 提取)
// ============================================================
static double calculateAccuracy(double passed_submits, double total_submits)
{
    return total_submits > 0 ? passed_submits / total_submits : 0.0;
}

// ============================================================
// 被测逻辑: should_cache = total_submits > 0
// (从 oj_model.hpp:3262 提取)
// ============================================================
static bool shouldCacheStats(int total_submits)
{
    return total_submits > 0;
}

// ============================================================
// 被测逻辑: is_pass 解析 - 空格分词检查所有 token
// (从 oj_server.cpp:364-384 提取，含我们的最后一个 token 修复)
// ============================================================
static bool parseIsPass(const std::string& status_str)
{
    if (status_str.empty())
        return false;
    bool is_pass = true;
    std::string token;
    for (char ch : status_str)
    {
        if (ch == ' ')
        {
            if (!token.empty() && token != "AC")
            {
                is_pass = false;
                break;
            }
            token.clear();
        }
        else
        {
            token += ch;
        }
    }
    // 检查最后一个 token
    if (is_pass && !token.empty() && token != "AC")
    {
        is_pass = false;
    }
    return is_pass;
}

// --- accuracy 测试 ---
static void test_accuracy_zero_submits()
{
    TEST("accuracy = 0 when total_submits == 0");
    double acc = calculateAccuracy(0, 0);
    ASSERT_FLOAT_EQ(acc, 0.0, 1e-9, "accuracy should be 0");
    PASS();
}

static void test_accuracy_all_passed()
{
    TEST("accuracy = 1.0 when all passed");
    double acc = calculateAccuracy(10, 10);
    ASSERT_FLOAT_EQ(acc, 1.0, 1e-9, "accuracy should be 1.0");
    PASS();
}

static void test_accuracy_half_passed()
{
    TEST("accuracy = 0.5 when half passed");
    double acc = calculateAccuracy(5, 10);
    ASSERT_FLOAT_EQ(acc, 0.5, 1e-9, "accuracy should be 0.5");
    PASS();
}

static void test_accuracy_zero_passed()
{
    TEST("accuracy = 0 when none passed");
    double acc = calculateAccuracy(0, 10);
    ASSERT_FLOAT_EQ(acc, 0.0, 1e-9, "accuracy should be 0");
    PASS();
}

// --- 缓存条件测试 ---
static void test_cache_disabled_for_zero()
{
    TEST("no cache when total_submits == 0");
    ASSERT_TRUE(!shouldCacheStats(0), "should not cache");
    PASS();
}

static void test_cache_enabled_for_positive()
{
    TEST("cache enabled when total_submits > 0");
    ASSERT_TRUE(shouldCacheStats(1), "should cache");
    PASS();
}

static void test_cache_enabled_for_large()
{
    TEST("cache enabled for large submission count");
    ASSERT_TRUE(shouldCacheStats(99999), "should cache");
    PASS();
}

// --- is_pass 解析测试 ---
static void test_parse_all_AC_with_trailing_space()
{
    TEST("parse 'AC AC AC ' (trailing space) -> all AC");
    ASSERT_EQ(parseIsPass("AC AC AC "), true,
              "expected true for all AC with trailing space");
    PASS();
}

static void test_parse_all_AC_without_trailing_space()
{
    TEST("parse 'AC AC AC' (no trailing space) -> all AC");
    ASSERT_EQ(parseIsPass("AC AC AC"), true,
              "expected true for all AC without trailing space");
    PASS();
}

static void test_parse_single_was()
{
    TEST("parse 'WA' -> not AC");
    ASSERT_EQ(parseIsPass("WA"), false,
              "expected false for WA");
    PASS();
}

static void test_parse_one_wa_middle()
{
    TEST("parse 'AC WA AC ' -> not all AC (WA in middle)");
    ASSERT_EQ(parseIsPass("AC WA AC "), false,
              "expected false for one WA");
    PASS();
}

static void test_parse_last_token_non_ac()
{
    TEST("parse 'AC AC WA' (last token is WA, no trailing space) -> not all AC");
    ASSERT_EQ(parseIsPass("AC AC WA"), false,
              "expected false when last token is WA without trailing space");
    PASS();
}

static void test_parse_last_token_non_ac_with_space()
{
    TEST("parse 'AC AC WA ' (last token is WA, trailing space) -> not all AC");
    ASSERT_EQ(parseIsPass("AC AC WA "), false,
              "expected false when last token is WA with trailing space");
    PASS();
}

static void test_parse_empty_string()
{
    TEST("parse empty string -> false");
    ASSERT_EQ(parseIsPass(""), false,
              "expected false for empty string");
    PASS();
}

static void test_parse_mixed_status_codes()
{
    TEST("parse 'AC AC TLE AC ' -> not all AC");
    ASSERT_EQ(parseIsPass("AC AC TLE AC "), false,
              "expected false for TLE");
    PASS();
}

static void test_parse_last_token_tle()
{
    TEST("parse 'AC AC TLE' (last token TLE, no trailing space) -> detected");
    ASSERT_EQ(parseIsPass("AC AC TLE"), false,
              "expected false for TLE as last token");
    PASS();
}

// ============================================================
int main()
{
    printf("\n=== 用户统计与 is_pass 解析单元测试 ===\n\n");
    printf("--- accuracy 计算 (oj_model.hpp:3214) ---\n");
    test_accuracy_zero_submits();
    test_accuracy_all_passed();
    test_accuracy_half_passed();
    test_accuracy_zero_passed();
    printf("\n--- 缓存条件 (oj_model.hpp:3262) ---\n");
    test_cache_disabled_for_zero();
    test_cache_enabled_for_positive();
    test_cache_enabled_for_large();
    printf("\n--- is_pass 解析 (oj_server.cpp:364-384 + 最后 token 修复) ---\n");
    test_parse_all_AC_with_trailing_space();
    test_parse_all_AC_without_trailing_space();
    test_parse_single_was();
    test_parse_one_wa_middle();
    test_parse_last_token_non_ac();
    test_parse_last_token_non_ac_with_space();
    test_parse_empty_string();
    test_parse_mixed_status_codes();
    test_parse_last_token_tle();
    printf("\n========================================\n");
    printf("结果: %d passed, %d failed, %d total\n\n",
           tests_passed, tests_failed, tests_passed + tests_failed);
    return tests_failed > 0 ? 1 : 0;
}
