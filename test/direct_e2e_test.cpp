// test/direct_e2e_test.cpp
// 端到端判题验证 — 绕过 MQ，直接测试 Docker 编译+运行+结果
//
// 流程: EventLoop → Judger::Run("hello world") → Docker编译 → Docker运行
//       → host Docker exec/wait → 协程恢复 → LOG_DEBUG 输出结果
//
// 构建: cd build && make direct_e2e_test
// 运行: ./output/direct_e2e_test

#include <iostream>
#include <thread>
#include <chrono>
#include <cstdlib>
#include <signal.h>

#include "../src/judge_service/include/event_loop.hpp"
#include "../src/judge_service/include/docker_sandbox.hpp"
#include "../src/judge_service/include/judger.hpp"
#include "../src/comm/logger.hpp"

using namespace oj::logger;

// ── main ────────────────────────────────────────────────

int main(int argc, char* argv[]) {
    std::cerr << "=== Direct E2E Judge Test ===" << std::endl;

    (void)argc;
    (void)argv;

    // 1. EventLoop
    auto loop = std::make_shared<oj::judge::JudgeEventLoop>();
    loop->Run(4);
    std::cerr << "[OK] EventLoop started (4 workers)" << std::endl;

    // 2. One-shot runner factory
    oj_sandbox::SandboxConfig runner_cfg;
    runner_cfg.image = "oj-runner:latest";
    runner_cfg.network_disabled = true;
    runner_cfg.read_only_root = true;
    oj_sandbox::SandboxPool pool(3, runner_cfg);
    std::cerr << "[OK] One-shot runner factory" << std::endl;

    // 4. Build test request — simple "hello world" C++ program
    oj::mq::JudgeTaskMessage req;
    req.set_message_id("direct-e2e-message");
    req.set_submission_id(100001);
    req.set_question_id("DIRECT_TEST");
    req.set_user_id(1);
    req.set_code(R"(#include <iostream>
int main() {
    std::cout << "hello world" << std::endl;
    return 0;
})");
    req.set_language("cpp");
    req.set_time_limit_ms(5000);
    req.set_memory_limit_mb(256);
    req.set_created_at(std::time(nullptr));

    // 5. Set test case (expect "hello world")
    std::vector<oj::judge::TestCase> test_cases;
    oj::judge::TestCase tc;
    tc.index = 1;
    tc.input = "";
    tc.output = "hello world";
    tc.cpu_limit = 2;   // seconds
    tc.mem_limit = 256; // MB
    test_cases.push_back(tc);
    auto* task_test = req.add_test_cases();
    task_test->set_index(1);
    task_test->set_input(tc.input);
    task_test->set_expected_output(tc.output);

    // 6. Create Judger and run
    oj::judge::Judger judger(&pool);
    judger.SetTestCases(test_cases);

    auto coro = judger.Run(req);
    judger.SetTask(coro);

    std::atomic<bool> done{false};
    std::atomic<bool> accepted{false};
    auto completion = [&done, &accepted](const oj::common::JudgeResult& result) {
        std::string out;
        result.SerializeToString(&out);
        LOG_DEBUG("JudgeResult bytes={}", out.size());
        std::cerr << "\n=== JUDGE COMPLETE ===" << std::endl;
        std::cerr << "status: " << result.status() << std::endl;
        accepted.store(result.status() == oj::common::SUBMISSION_STATUS_ACCEPTED);
        for (int i = 0; i < result.test_case_results_size(); ++i) {
            const auto& test_result = result.test_case_results(i);
            std::cerr << "test[" << i << "]: result=" << test_result.status()
                      << " time=" << test_result.time_used_ms() << "ms"
                      << " mem=" << test_result.memory_used_bytes() << "B" << std::endl;
        }
        done.store(true);
    };

    // 8. Push to event loop (moves the CoTask)
    std::string task_id = loop->PushNewTask(std::move(coro), 60000, std::move(completion));

    std::cerr << "[OK] Task pushed: " << task_id << std::endl;
    std::cerr << "Waiting for Docker compile+run (may take 10-30s)..." << std::endl;

    // 9. Wait for completion (with timeout)
    for (int i = 0; i < 60 && !done.load(); ++i) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        if (i % 5 == 0) std::cerr << "." << std::flush;
    }

    // 10. Shutdown
    pool.Shutdown();
    loop->Stop();

    if (done.load() && accepted.load()) {
        std::cerr << "\n[PASS] E2E test completed successfully!" << std::endl;
        return 0;
    } else {
        std::cerr << (done.load() ? "\n[FAIL] Judge returned a non-accepted result"
                                 : "\n[FAIL] Test timed out (60s)") << std::endl;
        return 1;
    }
}
