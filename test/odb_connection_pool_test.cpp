#include "comm/odb/connection_pool.hpp"

#include <algorithm>
#include <atomic>
#include <cassert>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

namespace {

using ns_odb::DatabasePtr;
using ns_odb::ODBConnectionPool;
using ns_odb::ODBPoolConfig;

DatabasePtr DisconnectedDatabase()
{
    return std::make_unique<odb::mysql::database>("", "", "");
}

ODBPoolConfig Config(unsigned int minimum, unsigned int maximum)
{
    ODBPoolConfig config;
    config.min_connections = minimum;
    config.max_connections = maximum;
    config.health_check_interval_ms = 10000;
    return config;
}

void TestMaximumConcurrencyAndCounts()
{
    ODBConnectionPool pool;
    std::atomic<int> created{0};
    pool.Init(Config(0, 2), nullptr,
              [&](const ODBPoolConfig&) { ++created; return DisconnectedDatabase(); },
              [](odb::mysql::database&, const ODBPoolConfig&) {},
              [](odb::mysql::database&) {});

    std::atomic<int> ready{0};
    std::atomic<int> peak{0};
    std::atomic<bool> release{false};
    std::vector<std::thread> threads;
    for (int i = 0; i < 8; ++i) {
        threads.emplace_back([&]() {
            auto db = pool.GetDatabase();
            assert(db);
            const int active = static_cast<int>(pool.ActiveCount());
            int previous_peak = peak.load();
            while (previous_peak < active &&
                   !peak.compare_exchange_weak(previous_peak, active)) {
            }
            ++ready;
            while (!release.load())
                std::this_thread::yield();
            pool.ReturnDatabase(std::move(db));
        });
    }

    while (ready.load() < 2)
        std::this_thread::yield();
    std::this_thread::sleep_for(std::chrono::milliseconds(30));
    assert(ready.load() == 2);
    assert(pool.ActiveCount() == 2);
    assert(created.load() == 2);
    release = true;
    for (auto& thread : threads)
        thread.join();
    assert(peak.load() <= 2);
    assert(pool.ActiveCount() == 0);
    assert(pool.IdleCount() == 2);
    pool.Shutdown();
}

void TestShutdownWakesWaitersAndHealthThread()
{
    ODBConnectionPool pool;
    pool.Init(Config(0, 1), nullptr,
              [](const ODBPoolConfig&) { return DisconnectedDatabase(); },
              [](odb::mysql::database&, const ODBPoolConfig&) {},
              [](odb::mysql::database&) {});
    auto held = pool.GetDatabase();
    std::atomic<bool> waiter_done{false};
    std::thread waiter([&]() {
        auto db = pool.GetDatabase();
        assert(!db);
        waiter_done = true;
    });

    const auto begin = std::chrono::steady_clock::now();
    pool.Shutdown();
    const auto elapsed = std::chrono::steady_clock::now() - begin;
    waiter.join();
    assert(waiter_done.load());
    assert(elapsed < std::chrono::milliseconds(500));
    assert(pool.ActiveCount() == 1);
    pool.ReturnDatabase(std::move(held));
    assert(pool.ActiveCount() == 0);
    assert(pool.IdleCount() == 0);
}

void TestCreateAndReplacementFailuresKeepCountsExact()
{
    {
        ODBConnectionPool pool;
        pool.Init(Config(0, 1), nullptr,
                  [](const ODBPoolConfig&) -> DatabasePtr { throw std::runtime_error("create"); },
                  [](odb::mysql::database&, const ODBPoolConfig&) {},
                  [](odb::mysql::database&) {});
        try {
            (void)pool.GetDatabase();
            assert(false);
        } catch (const std::runtime_error&) {
        }
        assert(pool.ActiveCount() == 0);
        assert(pool.IdleCount() == 0);
        pool.Shutdown();
    }

    {
        ODBConnectionPool pool;
        pool.Init(Config(0, 1), nullptr,
                  [](const ODBPoolConfig&) { return DisconnectedDatabase(); },
                  [](odb::mysql::database&, const ODBPoolConfig&) {
                      throw std::runtime_error("set names");
                  },
                  [](odb::mysql::database&) {});
        try {
            (void)pool.GetDatabase();
            assert(false);
        } catch (const std::runtime_error&) {
        }
        assert(pool.ActiveCount() == 0);
        assert(pool.IdleCount() == 0);
        pool.Shutdown();
    }

    {
        ODBConnectionPool pool;
        std::atomic<int> creates{0};
        pool.Init(Config(1, 1), nullptr,
                  [&](const ODBPoolConfig&) -> DatabasePtr {
                      if (++creates == 2)
                          throw std::runtime_error("replacement");
                      return DisconnectedDatabase();
                  },
                  [](odb::mysql::database&, const ODBPoolConfig&) {},
                  [](odb::mysql::database&) { throw std::runtime_error("health"); });
        try {
            (void)pool.GetDatabase();
            assert(false);
        } catch (const std::runtime_error&) {
        }
        assert(pool.ActiveCount() == 0);
        assert(pool.IdleCount() == 0);
        pool.Shutdown();
    }
}

void TestReplacementAndLatencyLabels()
{
    const auto directory = std::filesystem::temp_directory_path() / "oj_pool_latency_test";
    std::filesystem::remove_all(directory);
    std::filesystem::create_directories(directory);
    latecyMonitor::LatencyMonitor monitor;
    assert(monitor.setOutputFile(directory.string(), "pool.csv"));
    assert(monitor.start());
    monitor.enable(true);

    ODBConnectionPool pool;
    std::atomic<int> checks{0};
    pool.Init(Config(1, 1), &monitor,
              [](const ODBPoolConfig&) { return DisconnectedDatabase(); },
              [](odb::mysql::database&, const ODBPoolConfig&) {},
              [&](odb::mysql::database&) {
                  if (++checks == 1)
                      throw std::runtime_error("replace me");
              });
    auto db = pool.GetDatabase();
    assert(db);
    assert(pool.ActiveCount() == 1);
    pool.ReturnDatabase(std::move(db));
    assert(pool.ActiveCount() == 0);
    pool.Shutdown();
    monitor.stop();

    std::ifstream input(directory / "pool.csv");
    std::string csv((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
    for (const std::string label : {"DBPool.Acquire", "DBPool.Create", "DBPool.SetNames",
                                    "DBPool.HealthCheck.Select1", "DBPool.Replace"})
        assert(csv.find("," + label + ",") != std::string::npos);
}

void TestScopedDBMoveAssignmentReturnsOwnedConnection()
{
    ODBConnectionPool pool;
    pool.Init(Config(0, 2), nullptr,
              [](const ODBPoolConfig&) { return DisconnectedDatabase(); },
              [](odb::mysql::database&, const ODBPoolConfig&) {},
              [](odb::mysql::database&) {});
    {
        ns_odb::ScopedDB first(pool);
        ns_odb::ScopedDB second(pool);
        assert(pool.ActiveCount() == 2);
        first = std::move(second);
        assert(pool.ActiveCount() == 1);
        assert(second.Get() == nullptr);
    }
    assert(pool.ActiveCount() == 0);
    pool.Shutdown();
}

} // namespace

int main()
{
    TestMaximumConcurrencyAndCounts();
    TestShutdownWakesWaitersAndHealthThread();
    TestCreateAndReplacementFailuresKeepCountsExact();
    TestReplacementAndLatencyLabels();
    TestScopedDBMoveAssignmentReturnsOwnedConnection();
}
