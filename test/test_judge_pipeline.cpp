// test/test_judge_pipeline.cpp
// 判题链单元测试
//
// 测试内容：
// 1. 预处理器：代码写入文件
// 2. 编译器：正确/错误代码编译
// 3. 运行器：fork/exec 模式运行
// 4. 判题器：输出比较
// 5. 完整链路：预处理→编译→运行→判题

#include "../src/judge_service/include/pipeline/entry.hpp"
#include "../src/judge_service/include/pipeline/handler_base.hpp"
#include "../src/judge_service/include/pipeline/preprocesser.hpp"
#include "../src/judge_service/include/pipeline/compiler.hpp"
#include "../src/judge_service/include/pipeline/runner.hpp"
#include "../src/judge_service/include/pipeline/judge.hpp"

#include <cassert>
#include <iostream>
#include <string>
#include <fstream>
#include <sys/stat.h>
#include <jsoncpp/json/json.h>

using namespace ns_judge_pipeline;

// ==================== 辅助函数 ====================

static void EnsureTmpDir() {
    mkdir("../tmp", 0755);
}

static std::string RunJudge(const std::string& code,
                             const std::vector<TestCase>& tests = {},
                             bool is_custom_test = false) {
    HandlerEntry entry;

    if (!tests.empty()) {
        entry.SetTestCases(tests);
    }

    Json::Value in_json;
    in_json["code"] = code;
    in_json["id"] = "TEST";
    in_json["is_custom_test"] = is_custom_test;

    return entry.Run(in_json.toStyledString());
}

static std::string ExtractStatus(const std::string& json_str) {
    Json::Value root;
    Json::Reader reader;
    reader.parse(json_str, root);
    return root["status"].asString();
}

// ==================== 测试用例 ====================

/// 测试 1：状态码转换
void test_status_conversion() {
    std::cout << "[TEST] Status conversion...";

    assert(StatusToString(AC) == "AC");
    assert(StatusToString(WA) == "WA");
    assert(StatusToString(COMPILE_ERROR) == "CE");
    assert(StatusToString(TIME_LIMIT) == "TLE");
    assert(StatusToString(MEMORY_LIMIT) == "MLE");

    assert(ExitCodeToStatusCode(0) == AC);
    assert(ExitCodeToStatusCode(8) == FPE_ERROR);
    assert(ExitCodeToStatusCode(11) == SEGV_ERROR);
    assert(ExitCodeToStatusCode(24) == TIME_LIMIT);

    assert(StatusToDesc(AC) == "恭喜你!已通过此题");
    assert(StatusToDesc(COMPILE_ERROR) == "编译错误");

    std::cout << " PASSED" << std::endl;
}

/// 测试 2：编译器 — 正确代码编译成功
void test_compiler_success() {
    std::cout << "[TEST] Compiler success...";

    std::string code = R"(
#include <iostream>
int main() {
    int a, b;
    std::cin >> a >> b;
    std::cout << a + b << std::endl;
    return 0;
}
)";

    std::string result = RunJudge(code);
    std::string status = ExtractStatus(result);

    // 编译成功后，由于没有测试用例，可能返回 UNKNOWN 或 AC
    // 关键是不应该返回 CE
    assert(status.find("CE") == std::string::npos);

    std::cout << " PASSED (status=" << status << ")" << std::endl;
}

/// 测试 3：编译器 — 错误代码编译失败
void test_compiler_failure() {
    std::cout << "[TEST] Compiler failure...";

    std::string code = R"(
#include <iostream>
int main() {
    // 语法错误：缺少分号
    int a = 0
    return 0;
}
)";

    std::string result = RunJudge(code);
    std::string status = ExtractStatus(result);

    assert(status.find("CE") != std::string::npos);

    std::cout << " PASSED (status=" << status << ")" << std::endl;
}

/// 测试 4：完整链路 — A+B 问题 AC
void test_full_pipeline_ac() {
    std::cout << "[TEST] Full pipeline AC...";

    std::string code = R"(
#include <iostream>
int main() {
    int a, b;
    std::cin >> a >> b;
    std::cout << a + b << std::endl;
    return 0;
}
)";

    TestCase test;
    test.input = "3 4\n";
    test.output = "7\n";
    test.cpu_limit = 1;
    test.mem_limit = 256;

    std::string result = RunJudge(code, {test});
    std::string status = ExtractStatus(result);

    assert(status.find("AC") != std::string::npos);

    std::cout << " PASSED (status=" << status << ")" << std::endl;
}

/// 测试 5：完整链路 — WA（错误答案）
void test_full_pipeline_wa() {
    std::cout << "[TEST] Full pipeline WA...";

    std::string code = R"(
#include <iostream>
int main() {
    int a, b;
    std::cin >> a >> b;
    std::cout << a - b << std::endl;  // 故意写错：减法而非加法
    return 0;
}
)";

    TestCase test;
    test.input = "3 4\n";
    test.output = "7\n";
    test.cpu_limit = 1;
    test.mem_limit = 256;

    std::string result = RunJudge(code, {test});
    std::string status = ExtractStatus(result);

    assert(status.find("WA") != std::string::npos);

    std::cout << " PASSED (status=" << status << ")" << std::endl;
}

/// 测试 6：完整链路 — TLE（超时）
void test_full_pipeline_tle() {
    std::cout << "[TEST] Full pipeline TLE...";

    std::string code = R"(
#include <iostream>
int main() {
    while (true) {}  // 死循环
    return 0;
}
)";

    TestCase test;
    test.input = "";
    test.output = "";
    test.cpu_limit = 1;  // 1秒
    test.mem_limit = 256;

    std::string result = RunJudge(code, {test});
    std::string status = ExtractStatus(result);

    // 死循环应该触发 TLE 或 RTE
    assert(status.find("TLE") != std::string::npos ||
           status.find("RTE") != std::string::npos ||
           status.find("UNKNOWN") != std::string::npos);

    std::cout << " PASSED (status=" << status << ")" << std::endl;
}

/// 测试 7：完整链路 — 多个测试用例
void test_full_pipeline_multiple_tests() {
    std::cout << "[TEST] Full pipeline multiple tests...";

    std::string code = R"(
#include <iostream>
int main() {
    int a, b;
    std::cin >> a >> b;
    std::cout << a + b << std::endl;
    return 0;
}
)";

    std::vector<TestCase> tests;
    for (int i = 1; i <= 3; ++i) {
        TestCase t;
        t.input = std::to_string(i) + " " + std::to_string(i) + "\n";
        t.output = std::to_string(i * 2) + "\n";
        t.cpu_limit = 1;
        t.mem_limit = 256;
        tests.push_back(t);
    }

    std::string result = RunJudge(code, tests);
    std::string status = ExtractStatus(result);

    assert(status.find("AC") != std::string::npos);

    std::cout << " PASSED (status=" << status << ")" << std::endl;
}

/// 测试 8：链式处理器 — SetNext/Enable/Disable
void test_chain_control() {
    std::cout << "[TEST] Chain control...";

    auto preprocessor = std::make_shared<HandlerPreProcesser>();
    auto compiler = std::make_shared<HandlerCompiler>();

    preprocessor->SetNext(compiler);
    compiler->Disable();  // 禁用编译器

    // 禁用后应该直接跳过编译
    Json::Value in_json;
    in_json["code"] = "int main() { return 0; }";
    in_json["id"] = "TEST";

    std::string result = preprocessor->Execute("", in_json.toStyledString());

    // 不应该崩溃
    assert(!result.empty());

    std::cout << " PASSED" << std::endl;
}

// ==================== 主函数 ====================

int main() {
    std::cout << "=== Judge Pipeline Tests ===" << std::endl;

    EnsureTmpDir();

    test_status_conversion();
    test_chain_control();
    test_compiler_success();
    test_compiler_failure();
    test_full_pipeline_ac();
    test_full_pipeline_wa();
    test_full_pipeline_tle();
    test_full_pipeline_multiple_tests();

    std::cout << "=== All tests passed ===" << std::endl;
    return 0;
}