// test/test_docker_sandbox.cpp
// Docker Sandbox 单元测试
//
// 前置条件：
// 1. Docker Engine 运行中
// 2. oj-sandbox:latest 镜像已构建：
//    docker build -t oj-sandbox:latest -f docker/Dockerfile.sandbox .
// 3. 当前用户有权限访问 /var/run/docker.sock

#include "../src/judge_service/include/sandbox/docker_sandbox.hpp"
#include <cassert>
#include <iostream>
#include <string>

using namespace ns_sandbox;

/// 测试 1：容器创建和销毁
void test_create_destroy() {
    std::cout << "[TEST] Create and destroy container...";

    DockerSandbox sandbox;
    SandboxConfig config;
    config.image = "oj-sandbox:latest";
    config.memory_limit_mb = 256;
    config.cpu_limit_ms = 2000;
    config.timeout_ms = 5000;

    std::string id = sandbox.Create(config);
    assert(!id.empty());
    assert(sandbox.IsRunning());

    sandbox.Destroy();
    assert(!sandbox.IsRunning());

    std::cout << " PASSED" << std::endl;
}

/// 测试 2：在容器内执行命令
void test_exec_command() {
    std::cout << "[TEST] Execute command in container...";

    DockerSandbox sandbox;
    SandboxConfig config;
    config.image = "oj-sandbox:latest";

    std::string id = sandbox.Create(config);
    assert(!id.empty());
    sandbox.Start();

    // 执行 echo 命令
    ExecResult result = sandbox.Exec("echo hello_world", 5000);
    assert(result.exit_code == 0);

    sandbox.Destroy();

    std::cout << " PASSED" << std::endl;
}

/// 测试 3：写入和读取文件
void test_write_read_file() {
    std::cout << "[TEST] Write and read file...";

    DockerSandbox sandbox;
    SandboxConfig config;
    config.image = "oj-sandbox:latest";

    sandbox.Create(config);
    sandbox.Start();

    // 写入文件
    std::string content = "int main() { return 0; }";
    sandbox.WriteFile("/home/judge/test.cpp", content);

    // 读取文件
    std::string read_content = sandbox.ReadFile("/home/judge/test.cpp");
    assert(read_content.find("int main") != std::string::npos);

    sandbox.Destroy();

    std::cout << " PASSED" << std::endl;
}

/// 测试 4：在容器内编译和运行 C++ 代码
void test_compile_and_run() {
    std::cout << "[TEST] Compile and run C++ code...";

    DockerSandbox sandbox;
    SandboxConfig config;
    config.image = "oj-sandbox:latest";
    config.memory_limit_mb = 256;
    config.cpu_limit_ms = 2000;

    sandbox.Create(config);
    sandbox.Start();

    // 写入源代码
    std::string code = R"(
#include <iostream>
int main() {
    std::cout << "Hello from sandbox!" << std::endl;
    return 0;
}
)";
    sandbox.WriteFile("/home/judge/main.cpp", code);

    // 编译
    ExecResult compile_result = sandbox.Exec("g++ -o main main.cpp", 10000);
    assert(compile_result.exit_code == 0);

    // 运行
    ExecResult run_result = sandbox.Exec("./main", 5000);
    assert(run_result.exit_code == 0);
    assert(run_result.stdout_str.find("Hello from sandbox!") != std::string::npos);

    sandbox.Destroy();

    std::cout << " PASSED" << std::endl;
}

/// 测试 5：资源限制 — 内存超限
void test_memory_limit() {
    std::cout << "[TEST] Memory limit...";

    DockerSandbox sandbox;
    SandboxConfig config;
    config.image = "oj-sandbox:latest";
    config.memory_limit_mb = 32;  // 32MB 限制

    sandbox.Create(config);
    sandbox.Start();

    // 写入会申请大量内存的代码
    std::string code = R"(
#include <cstdlib>
int main() {
    void* p = malloc(256 * 1024 * 1024);  // 申请 256MB
    if (p) free(p);
    return 0;
}
)";
    sandbox.WriteFile("/home/judge/mem_test.cpp", code);

    // 编译
    sandbox.Exec("g++ -o mem_test mem_test.cpp", 10000);

    // 运行（应该因内存超限而失败）
    ExecResult result = sandbox.Exec("./mem_test", 5000);
    assert(result.exit_code != 0);  // 应该非零退出

    sandbox.Destroy();

    std::cout << " PASSED" << std::endl;
}

/// 测试 6：沙箱池
void test_sandbox_pool() {
    std::cout << "[TEST] Sandbox pool...";

    SandboxConfig config;
    config.image = "oj-sandbox:latest";
    SandboxPool pool(2, config);

    // 获取沙箱
    auto sandbox1 = pool.Acquire();
    assert(sandbox1 != nullptr);
    assert(sandbox1->IsRunning());

    auto sandbox2 = pool.Acquire();
    assert(sandbox2 != nullptr);

    // 归还
    pool.Release(std::move(sandbox1));
    pool.Release(std::move(sandbox2));

    // 再次获取（应该从池中复用）
    auto sandbox3 = pool.Acquire();
    assert(sandbox3 != nullptr);
    assert(sandbox3->IsRunning());

    pool.Release(std::move(sandbox3));
    pool.Shutdown();

    std::cout << " PASSED" << std::endl;
}

// ==================== 主函数 ====================

int main() {
    std::cout << "=== Docker Sandbox Tests ===" << std::endl;

    try {
        test_create_destroy();
        test_exec_command();
        test_write_read_file();
        test_compile_and_run();
        test_memory_limit();
        test_sandbox_pool();
    } catch (const std::exception& e) {
        std::cerr << "Test failed: " << e.what() << std::endl;
        return 1;
    }

    std::cout << "=== All tests passed ===" << std::endl;
    return 0;
}