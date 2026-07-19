#include <atomic>
#include <cassert>
#include <chrono>
#include <iostream>
#include <thread>

#include "../src/judge_service/include/event_loop.hpp"

namespace {

oj::judge::CoTask TwoStageTask(std::atomic<int>& stages)
{
    auto first = co_await oj::judge::DockerTaskAwaitable{
        1000,
        [&stages] {
            std::this_thread::sleep_for(std::chrono::milliseconds(30));
            ++stages;
            return oj::judge::DockerTaskResult{"OK", 0};
        }};
    assert(first.status == "OK");

    auto second = co_await oj::judge::DockerTaskAwaitable{
        1000,
        [&stages] {
            std::this_thread::sleep_for(std::chrono::milliseconds(30));
            ++stages;
            return oj::judge::DockerTaskResult{"OK", 0};
        }};
    assert(second.exit_code == 0);

    auto result = std::make_shared<oj::common::JudgeResult>();
    result->set_status(oj::common::SUBMISSION_STATUS_ACCEPTED);
    co_return result;
}

oj::judge::CoTask SlowTask()
{
    co_await oj::judge::DockerTaskAwaitable{
        1000,
        [] {
            std::this_thread::sleep_for(std::chrono::milliseconds(80));
            return oj::judge::DockerTaskResult{"OK", 0};
        }};
    auto result = std::make_shared<oj::common::JudgeResult>();
    result->set_status(oj::common::SUBMISSION_STATUS_ACCEPTED);
    co_return result;
}

oj::judge::CoTask IoTimeoutTask(std::atomic<bool>& saw_timeout)
{
    auto io = co_await oj::judge::DockerTaskAwaitable{
        20,
        [] {
            std::this_thread::sleep_for(std::chrono::milliseconds(70));
            return oj::judge::DockerTaskResult{"OK", 0};
        }};
    saw_timeout.store(io.timed_out && io.status == "TIME_LIMIT");
    auto result = std::make_shared<oj::common::JudgeResult>();
    result->set_status(oj::common::SUBMISSION_STATUS_ACCEPTED);
    co_return result;
}

oj::judge::CoTask StopWhileRunningTask(std::atomic<bool>& entered,
                                       std::atomic<bool>& io_started)
{
    entered.store(true);
    std::this_thread::sleep_for(std::chrono::milliseconds(60));
    co_await oj::judge::DockerTaskAwaitable{
        1000,
        [&io_started] {
            io_started.store(true);
            return oj::judge::DockerTaskResult{"OK", 0};
        }};
    auto result = std::make_shared<oj::common::JudgeResult>();
    result->set_status(oj::common::SUBMISSION_STATUS_ACCEPTED);
    co_return result;
}

bool WaitFor(const std::function<bool()>& condition, int timeout_ms)
{
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout_ms);
    while (std::chrono::steady_clock::now() < deadline) {
        if (condition()) return true;
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    return condition();
}

} // namespace

int main()
{
    {
        oj::judge::JudgeEventLoop loop;
        loop.Run(2);
        std::atomic<int> stages{0};
        std::atomic<int> callbacks{0};
        loop.PushNewTask(TwoStageTask(stages), 5000,
            [&callbacks](const oj::common::JudgeResult& result) {
                assert(result.status() == oj::common::SUBMISSION_STATUS_ACCEPTED);
                ++callbacks;
            });

        assert(WaitFor([&] { return stages.load() >= 1; }, 1000));
        assert(loop.PendingCount() == 1);
        assert(WaitFor([&] { return callbacks.load() == 1; }, 1000));
        assert(WaitFor([&] { return loop.PendingCount() == 0; }, 1000));
        assert(stages.load() == 2);
        loop.Stop();
    }

    {
        oj::judge::JudgeEventLoop loop;
        loop.Run(1);
        std::atomic<int> callbacks{0};
        std::atomic<int> status{oj::common::SUBMISSION_STATUS_UNSPECIFIED};
        loop.PushNewTask(SlowTask(), 5000,
            [&callbacks, &status](const oj::common::JudgeResult& result) {
                status.store(result.status());
                ++callbacks;
            });
        assert(WaitFor([&] { return loop.PendingCount() == 1; }, 200));
        loop.Stop();
        assert(callbacks.load() == 1);
        assert(status.load() == oj::common::SUBMISSION_STATUS_CANCELLED);
        assert(loop.PendingCount() == 0);
    }

    {
        oj::judge::JudgeEventLoop loop;
        loop.Run(1);
        std::atomic<bool> saw_timeout{false};
        std::atomic<int> callbacks{0};
        loop.PushNewTask(IoTimeoutTask(saw_timeout), 1000,
            [&callbacks](const oj::common::JudgeResult& result) {
                assert(result.status() == oj::common::SUBMISSION_STATUS_ACCEPTED);
                ++callbacks;
            });
        assert(WaitFor([&] { return callbacks.load() == 1; }, 500));
        assert(saw_timeout.load());
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        assert(callbacks.load() == 1);
        loop.Stop();
    }

    {
        oj::judge::JudgeEventLoop loop;
        loop.Run(1);
        std::atomic<int> callbacks{0};
        std::atomic<int> status{oj::common::SUBMISSION_STATUS_UNSPECIFIED};
        loop.PushNewTask(SlowTask(), 20,
            [&callbacks, &status](const oj::common::JudgeResult& result) {
                status.store(result.status());
                ++callbacks;
            });
        assert(WaitFor([&] { return callbacks.load() == 1; }, 100));
        assert(status.load() == oj::common::SUBMISSION_STATUS_CANCELLED);
        assert(WaitFor([&] { return loop.PendingCount() == 0; }, 300));
        std::this_thread::sleep_for(std::chrono::milliseconds(30));
        assert(callbacks.load() == 1);
        loop.Stop();
    }

    {
        oj::judge::JudgeEventLoop loop;
        loop.Run(1);
        std::atomic<bool> entered{false};
        std::atomic<bool> io_started{false};
        std::atomic<int> callbacks{0};
        loop.PushNewTask(StopWhileRunningTask(entered, io_started), 1000,
            [&callbacks](const oj::common::JudgeResult& result) {
                assert(result.status() == oj::common::SUBMISSION_STATUS_CANCELLED);
                ++callbacks;
            });
        assert(WaitFor([&] { return entered.load(); }, 1000));
        loop.Stop();
        assert(!io_started.load());
        assert(callbacks.load() == 1);
        assert(loop.PendingCount() == 0);
    }

    std::cout << "judge_event_loop_test passed\n";
    return 0;
}
