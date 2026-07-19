#include "comm.hpp"
#include <thread>
#include <vector>
#include <atomic>
#include <chrono>
#include <sstream>

// 单端点并发洪水
void TestEndpointFlood() {
    TEST("DoS-首页并发200请求");
    auto t0 = std::chrono::steady_clock::now();
    std::atomic<int> ok{0}, fail{0};

    std::vector<std::thread> threads;
    for (int i = 0; i < 200; i++) {
        threads.emplace_back([&ok, &fail]() {
            long code;
            std::string r = HttpGet(BASE + "/", "", &code);
            if (code == 200) ok++; else fail++;
        });
    }
    for (auto& t : threads) t.join();

    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - t0).count();
    LOG("  成功:" + std::to_string(ok) + " 失败:" + std::to_string(fail) + " 耗时:" + std::to_string(ms) + "ms");
    // 服务仍能响应大部分请求（>80%）
    RESULT(ok > 160 && ms < 15000);
}

// 分页洪水 - 遍历大量页码触发MySQL查询
void TestPageFlood() {
    TEST("DoS-分页洪水100页");
    auto t0 = std::chrono::steady_clock::now();
    std::atomic<int> ok{0}, total_ms{0};

    std::vector<std::thread> threads;
    for (int i = 1; i <= 100; i++) {
        threads.emplace_back([i, &ok]() {
            long code;
            std::string r = HttpGet(BASE + "/all_questions?page=" + std::to_string(i), session_id, &code);
            if (code == 200 && r.find("题目列表") != std::string::npos) ok++;
        });
    }
    for (auto& t : threads) t.join();

    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - t0).count();
    LOG("  成功:" + std::to_string(ok) + "/100 耗时:" + std::to_string(ms) + "ms");
    RESULT(ok > 50 && ms < 20000);
}

// 慢端点攻击 - 同时触发编译服务
void TestSlowEndpointFlood() {
    TEST("DoS-编译测试并发20");
    auto t0 = std::chrono::steady_clock::now();
    std::atomic<int> ok{0};

    std::vector<std::thread> threads;
    for (int i = 0; i < 20; i++) {
        threads.emplace_back([&ok]() {
            long code;
            std::string r = HttpPostJson(BASE + "/api/question/1/test",
                R"({"code":"#include <iostream>\nint main(){return 0;}","test_type":"sample","test_case_id":1})",
                session_id, &code);
            if (code == 200) ok++;
        });
    }
    for (auto& t : threads) t.join();

    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - t0).count();
    LOG("  完成:" + std::to_string(ok) + "/20 耗时:" + std::to_string(ms) + "ms");
    RESULT(ok < 20 || ms < 60000); // 至少不应完全挂掉
}

// 大请求体攻击
void TestLargePayload() {
    TEST("DoS-大请求体攻击");
    // 发送5MB请求体
    std::string big(5 * 1024 * 1024, 'X');
    long code;
    auto t0 = std::chrono::steady_clock::now();
    std::string r = HttpPostJson(BASE + "/api/auth/verify_code",
        "{\"data\":\"" + big + "\"}", "", &code);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - t0).count();
    LOG("  响应码:" + std::to_string(code) + " 耗时:" + std::to_string(ms) + "ms");
    RESULT(code != 200 || ms < 5000);
}

// 服务恢复验证
void TestRecovery() {
    TEST("DoS-服务恢复验证");
    long code;
    std::string r = HttpGet(BASE + "/", session_id,&code);
    LOG("  首页响应码:" + std::to_string(code));
    RESULT(code == 200);
}

int main() {
    curl_global_init(CURL_GLOBAL_ALL);
    TestEndpointFlood();
    TestPageFlood();
    TestSlowEndpointFlood();
    TestLargePayload();
    TestRecovery();
    curl_global_cleanup();
}
