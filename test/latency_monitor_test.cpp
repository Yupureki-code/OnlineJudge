#include "comm/latecymonitor.hpp"

#include <atomic>
#include <cassert>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <thread>
#include <vector>

namespace {

std::filesystem::path TestDirectory()
{
    auto path = std::filesystem::temp_directory_path() / "oj_latency_monitor_test";
    std::filesystem::remove_all(path);
    std::filesystem::create_directories(path);
    return path;
}

std::vector<std::string> ReadLines(const std::filesystem::path& path)
{
    std::ifstream input(path);
    std::vector<std::string> lines;
    for (std::string line; std::getline(input, line);)
        lines.push_back(std::move(line));
    return lines;
}

void TestStopDrainsCsvRecords()
{
    const auto directory = TestDirectory();
    latecyMonitor::LatencyMonitor monitor;
    assert(monitor.setOutputFile(directory.string(), "drain.csv"));
    monitor.set_batch_size(64);
    monitor.set_flush_interval(std::chrono::seconds(10));
    assert(monitor.start());
    monitor.enable(true);

    for (int i = 0; i < 25; ++i)
        monitor.record("Drain.Operation", i);
    monitor.stop();

    const auto lines = ReadLines(directory / "drain.csv");
    assert(lines.size() == 25);
    for (const auto& line : lines) {
        const auto first = line.find(',');
        const auto second = line.find(',', first + 1);
        assert(first != std::string::npos);
        assert(second != std::string::npos);
        assert(line.substr(first + 1, second - first - 1) == "Drain.Operation");
    }
    assert(monitor.queue_size() == 0);
    assert(!monitor.is_running());
}

void TestRestartAndConcurrentConfiguration()
{
    const auto directory = TestDirectory();
    latecyMonitor::LatencyMonitor monitor;
    assert(monitor.setOutputFile(directory.string(), "restart.csv"));

    for (int run = 0; run < 2; ++run) {
        assert(monitor.start());
        monitor.enable(true);
        std::atomic<bool> done{false};
        std::thread configure([&]() {
            for (int i = 1; i <= 200; ++i) {
                monitor.set_batch_size(static_cast<size_t>(i % 11));
                monitor.set_flush_interval(std::chrono::milliseconds((i % 7) + 1));
            }
            done = true;
        });
        while (!done.load())
            monitor.record("Concurrent.Config", run);
        configure.join();
        monitor.record("Restart.Record", run);
        monitor.stop();
    }

    const auto lines = ReadLines(directory / "restart.csv");
    assert(lines.size() >= 2);
}

void TestEnvironmentConfiguration()
{
    const auto directory = TestDirectory();
    setenv("OJ_LATENCY_PATH", directory.c_str(), 1);
    setenv("OJ_LATENCY_BATCH_SIZE", "17", 1);
    setenv("OJ_LATENCY_FLUSH_MS", "23", 1);

    const auto config = latecyMonitor::LatencyConfigFromEnv("oj_server");
    assert(config.output_path == directory.string());
    assert(config.file_name == "oj_server_latency.csv");
    assert(config.batch_size == 17);
    assert(config.flush_interval == std::chrono::milliseconds(23));

    setenv("OJ_LATENCY_BATCH_SIZE", "-1", 1);
    setenv("OJ_LATENCY_FLUSH_MS", "0", 1);
    const auto fallback = latecyMonitor::LatencyConfigFromEnv("oj_admin");
    assert(fallback.file_name == "oj_admin_latency.csv");
    assert(fallback.batch_size > 0);
    assert(fallback.flush_interval.count() > 0);

    unsetenv("OJ_LATENCY_PATH");
    unsetenv("OJ_LATENCY_BATCH_SIZE");
    unsetenv("OJ_LATENCY_FLUSH_MS");
}

void TestFileFailureDoesNotPreventStop()
{
    latecyMonitor::LatencyMonitor monitor;
    assert(monitor.setOutputFile("/proc/oj-latency-not-writable", "failure.csv"));
    assert(monitor.start());
    monitor.enable(true);
    monitor.record("File.Failure", 1);
    monitor.stop();
    assert(!monitor.is_running());
    assert(monitor.queue_size() == 0);
}

} // namespace

int main()
{
    TestStopDrainsCsvRecords();
    TestRestartAndConcurrentConfiguration();
    TestEnvironmentConfiguration();
    TestFileFailureDoesNotPreventStop();
}
