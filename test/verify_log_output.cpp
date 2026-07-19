// test/verify_log_output.cpp
// 直接验证判题逻辑 + LOG_DEBUG 输出
// 无需 Docker — 本地编译运行，模拟完整判题流程

#include <iostream>
#include <cstdlib>
#include <cstdio>
#include <sys/wait.h>
#include <unistd.h>
#include "../src/comm/logger.hpp"
#include "../src/comm/proto/judge_service.pb.h"

using namespace oj::logger;

int main() {
    // Initialize logger (required before LOG_DEBUG)
    oj::logger::InitLogger("verify", "", spdlog::level::debug);
    
    std::cerr << "=== Judge Logic + LOG_DEBUG Output Verification ===" << std::endl;

    // ── Step 1: Write "hello world" C++ source ──
    const char* code = R"(#include <iostream>
int main() {
    std::cout << "hello world" << std::endl;
    return 0;
})";

    std::string src_path = "/tmp/test_hello.cpp";
    std::string exe_path = "/tmp/test_hello.exe";
    
    FILE* f = fopen(src_path.c_str(), "w");
    fprintf(f, "%s", code);
    fclose(f);
    std::cerr << "[1] Source written: " << src_path << std::endl;

    // ── Step 2: Compile ──
    std::string compile_cmd = "g++ -o " + exe_path + " " + src_path + " 2>/tmp/compile_err";
    int cr = system(compile_cmd.c_str());
    bool compile_ok = (WIFEXITED(cr) && WEXITSTATUS(cr) == 0);
    std::cerr << "[2] Compile: " << (compile_ok ? "OK" : "FAILED") << std::endl;
    if (!compile_ok) {
        std::cerr << "  Compile error!" << std::endl;
        return 1;
    }

    // ── Step 3: Run and capture output ──
    std::string run_cmd = exe_path + " > /tmp/user_output.txt 2>/tmp/user_stderr.txt";
    int rr = system(run_cmd.c_str());
    int exit_code = WIFEXITED(rr) ? WEXITSTATUS(rr) : -1;
    std::cerr << "[3] Run: exit_code=" << exit_code << std::endl;

    // ── Step 4: Read user output (equivalent to runner_sandbox->ReadFile) ──
    FILE* of = fopen("/tmp/user_output.txt", "r");
    char buf[4096] = {};
    fread(buf, 1, sizeof(buf)-1, of);
    fclose(of);
    std::string user_output(buf);
    // Trim trailing newline
    while (!user_output.empty() && (user_output.back() == '\n' || user_output.back() == '\r'))
        user_output.pop_back();
    
    // Verify parameterized logging without exposing program output.
    LOG_DEBUG("user program output_bytes={}", user_output.size());
    std::cerr << "[4] user_output = '" << user_output << "'" << std::endl;
    std::cerr << "    Expected:   'hello world'" << std::endl;
    std::cerr << "    Match: " << (user_output == "hello world" ? "✓ YES" : "✗ NO") << std::endl;

    // ── Step 5: Build JudgeFinishedRequest ──
    oj_judge::JudgeFinishedRequest result;
    result.set_submission_id(100001);
    result.set_question_id("HELLO_TEST");
    result.set_user_id(1);
    auto* st = result.add_status_list();
    st->set_result(user_output == "hello world" ? oj_judge::AC : oj_judge::WA);
    st->set_time_cost(42);
    st->set_memory_cost(1024);

    // Verify result metadata logging without exposing the serialized body.
    std::string out;
    result.SerializeToString(&out);
    LOG_DEBUG("JudgeFinishedRequest submission_id={} bytes={}", result.submission_id(), out.size());
    
    // Also print human-readable form
    std::cerr << "[5] JudgeFinishedRequest:" << std::endl;
    std::cerr << "    submission_id: " << result.submission_id() << std::endl;
    std::cerr << "    question_id: " << result.question_id() << std::endl;
    std::cerr << "    user_id: " << result.user_id() << std::endl;
    std::cerr << "    status_list[0].result: " << st->result() 
              << " (" << (st->result() == oj_judge::AC ? "AC" : "WA") << ")" << std::endl;
    std::cerr << "    Protobuf size: " << out.size() << " bytes" << std::endl;

    // ── Verify printf-style output as well ──
    printf("\n=== LOG_DEBUG output (check stderr above) ===\n");
    printf("Expected LOG_DEBUG lines:\n");
    printf("  user_code_output: hello world\n");
    printf("  JudgeFinishedRequest : <protobuf binary>\n");

    return (user_output == "hello world") ? 0 : 1;
}
