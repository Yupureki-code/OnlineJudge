#include <iostream>
#include <spdlog/common.h>
#include <spdlog/spdlog.h>

int main()
{
    spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [thread %t] %v");
    spdlog::set_level(spdlog::level::debug);
    std::string name = "test_logger";
    int num = 100;
    double pi = 3.14159;
    spdlog::info("Hello, world1!");
    spdlog::error("Hello, {}!", "world");
    spdlog::debug("Hello, {}!", "world");
    spdlog::warn("Hello, {}!", "world");
    spdlog::info("Hello, {0},{1:.1f}!", name,pi);
    return 0;
}