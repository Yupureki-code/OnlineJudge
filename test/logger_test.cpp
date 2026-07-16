#include "comm/logger.hpp"

#include <cassert>
#include <filesystem>
#include <fstream>
#include <string>
#include <thread>
#include <vector>

namespace {

std::string ReadFile(const std::filesystem::path& path)
{
    std::ifstream input(path);
    return std::string(std::istreambuf_iterator<char>(input),
                       std::istreambuf_iterator<char>());
}

std::size_t CountOccurrences(const std::string& text, const std::string& needle)
{
    std::size_t count = 0;
    for (std::size_t position = 0;
         (position = text.find(needle, position)) != std::string::npos;
         position += needle.size()) {
        ++count;
    }
    return count;
}

void TestPreInitAndRepeatedShutdown()
{
    oj::logger::ShutdownLogger();
    LOG_INFO("pre-init fallback {}", 1);
    oj::logger::FlushLogger();
    oj::logger::ShutdownLogger();
    oj::logger::ShutdownLogger();
}

void TestFileLoggerLifecycle()
{
    const auto root = std::filesystem::temp_directory_path() / "oj_logger_test";
    std::filesystem::remove_all(root);
    const auto first_file = root / "first" / "server.log";

    assert(oj::logger::InitLogger("logger-test", first_file.string(), spdlog::level::debug));
    LOG_INFO("first message {}", 7);
    assert(oj::logger::InitLogger("logger-test", first_file.string(), spdlog::level::debug));
    LOG_WARNING("duplicate init message");

    constexpr int thread_count = 8;
    constexpr int messages_per_thread = 100;
    std::vector<std::thread> writers;
    for (int thread = 0; thread < thread_count; ++thread) {
        writers.emplace_back([thread]() {
            for (int message = 0; message < messages_per_thread; ++message)
                LOG_INFO("concurrent thread={} message={}", thread, message);
        });
    }
    for (auto& writer : writers)
        writer.join();

    oj::logger::FlushLogger();
    const std::string first = ReadFile(first_file);
    assert(first.find("[logger-test]") != std::string::npos);
    assert(first.find("logger_test.cpp:") != std::string::npos);
    assert(first.find("first message 7") != std::string::npos);
    assert(CountOccurrences(first, "duplicate init message") == 1);
    assert(CountOccurrences(first, "concurrent thread=") ==
           thread_count * messages_per_thread);
    for (int thread = 0; thread < thread_count; ++thread) {
        for (int message = 0; message < messages_per_thread; ++message) {
            const std::string expected = "concurrent thread=" + std::to_string(thread) +
                                         " message=" + std::to_string(message);
            assert(first.find(expected) != std::string::npos);
        }
    }

    const auto second_file = root / "second" / "judge.log";
    assert(oj::logger::InitLogger("judge-test", second_file.string(), spdlog::level::info));
    LOG_INFO("second logger message");
    oj::logger::ShutdownLogger();

    const std::string second = ReadFile(second_file);
    assert(second.find("[judge-test]") != std::string::npos);
    assert(second.find("second logger message") != std::string::npos);
    assert(first.find("second logger message") == std::string::npos);
}

} // namespace

int main()
{
    TestPreInitAndRepeatedShutdown();
    TestFileLoggerLifecycle();
}
