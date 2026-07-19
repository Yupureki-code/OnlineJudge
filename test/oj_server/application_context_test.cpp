#include "runtime/application_context.hpp"
#include "runtime/business_executor.hpp"

#include <atomic>
#include <cassert>
#include <chrono>
#include <condition_variable>
#include <exception>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

namespace {

using oj::runtime::ApplicationContext;
using oj::runtime::BusinessExecutor;
using oj::runtime::LifecycleEvent;
using oj::runtime::LifecycleStage;
using oj::runtime::MqService;
using oj::runtime::OdbPoolService;
using oj::runtime::StopPolicy;
using oj::runtime::SubmitResult;

class FakeOdbPool final : public OdbPoolService
{
public:
    void Start(const ns_odb::ODBPoolConfig&, latecyMonitor::LatencyMonitor&) override
    {
        ++starts;
        if (fail_start)
            throw std::runtime_error("ODB start failed");
    }

    void Stop() noexcept override { ++stops; }

    int starts = 0;
    int stops = 0;
    bool fail_start = false;
};

class FakeMq final : public MqService
{
public:
    void Start() override
    {
        ++starts;
        if (fail_start)
            throw std::runtime_error("MQ start failed");
    }

    void Stop() noexcept override { ++stops; }
    bool Publish(const std::string&) override { return true; }

    int starts = 0;
    int stops = 0;
    bool fail_start = false;
};

void TestExecutorSaturationAndFifoDrain()
{
    BusinessExecutor executor({.worker_count = 1, .queue_capacity = 1});
    assert(executor.Start());

    std::mutex mutex;
    std::condition_variable cv;
    bool first_running = false;
    bool release_first = false;
    std::vector<int> completed;

    assert(executor.Submit([&] {
               {
                   std::lock_guard lock(mutex);
                   first_running = true;
               }
               cv.notify_all();
               std::unique_lock lock(mutex);
               cv.wait(lock, [&] { return release_first; });
               completed.push_back(1);
           }) == SubmitResult::Accepted);

    {
        std::unique_lock lock(mutex);
        cv.wait(lock, [&] { return first_running; });
    }
    assert(executor.Submit([&] { completed.push_back(2); }) == SubmitResult::Accepted);
    assert(executor.Submit([] {}) == SubmitResult::QueueFull);
    auto saturated = executor.GetSnapshot();
    assert(saturated.worker_count == 1);
    assert(saturated.queue_capacity == 1);
    assert(saturated.active_workers == 1);
    assert(saturated.queue_depth == 1);
    assert(saturated.accepted_count == 2);
    assert(saturated.rejected_queue_full_count == 1);

    {
        std::lock_guard lock(mutex);
        release_first = true;
    }
    cv.notify_all();
    executor.Stop(StopPolicy::Drain);

    assert((completed == std::vector<int>{1, 2}));
    assert(executor.Submit([] {}) == SubmitResult::Stopped);
    auto drained = executor.GetSnapshot();
    assert(drained.completed_count == 2);
    assert(drained.rejected_stopped_count == 1);
    assert(drained.queue_wait_count == 2);
    assert(drained.execution_count == 2);
    assert(drained.execution_total_us >= drained.execution_max_us);
    executor.Stop(StopPolicy::CancelPending);
}

void TestExecutorCancelPending()
{
    BusinessExecutor executor({.worker_count = 1, .queue_capacity = 2});
    assert(executor.Start());

    std::mutex mutex;
    std::condition_variable cv;
    bool running = false;
    bool release = false;
    std::atomic<int> pending_runs{0};

    assert(executor.Submit([&] {
               {
                   std::lock_guard lock(mutex);
                   running = true;
               }
               cv.notify_all();
               std::unique_lock lock(mutex);
               cv.wait(lock, [&] { return release; });
           }) == SubmitResult::Accepted);
    {
        std::unique_lock lock(mutex);
        cv.wait(lock, [&] { return running; });
    }
    assert(executor.Submit([&] { ++pending_runs; }) == SubmitResult::Accepted);

    std::thread stopper([&] { executor.Stop(StopPolicy::CancelPending); });
    while (executor.IsAccepting())
        std::this_thread::yield();
    {
        std::lock_guard lock(mutex);
        release = true;
    }
    cv.notify_all();
    stopper.join();
    assert(pending_runs.load() == 0);
    auto cancelled = executor.GetSnapshot();
    assert(cancelled.completed_count == 1);
    assert(cancelled.cancelled_pending_count == 1);
}

void TestExecutorExceptionIsolation()
{
    std::atomic<int> exceptions{0};
    std::atomic<int> completed{0};
    BusinessExecutor executor(
        {.worker_count = 1, .queue_capacity = 2},
        [&](std::exception_ptr error) {
            try {
                std::rethrow_exception(error);
            } catch (const std::runtime_error&) {
                ++exceptions;
                throw std::logic_error("exception hook failed");
            }
        });
    assert(executor.Start());
    assert(executor.Submit([] { throw std::runtime_error("task failed"); }) ==
           SubmitResult::Accepted);
    assert(executor.Submit([&] { ++completed; }) == SubmitResult::Accepted);
    executor.Stop(StopPolicy::Drain);
    assert(exceptions.load() == 1);
    assert(completed.load() == 1);
    auto metrics = executor.GetSnapshot();
    assert(metrics.completed_count == 2);
    assert(metrics.execution_count == 2);
}

ApplicationContext::Config ContextConfig()
{
    ApplicationContext::Config config;
    config.process_name = "application_context_test";
    config.monitor.output_path = "/tmp/oj_application_context_test";
    config.monitor.file_name = "latency.csv";
    config.executor.worker_count = 1;
    config.executor.queue_capacity = 2;
    return config;
}

void TestContextLifecycleOrderAndRepeatedStop()
{
    FakeOdbPool odb;
    FakeMq mq;
    std::vector<std::string> order;
    auto callback = [&](LifecycleStage stage, LifecycleEvent event) {
        order.push_back(oj::runtime::ToString(stage) + ":" + oj::runtime::ToString(event));
    };

    ApplicationContext context(
        ContextConfig(), odb, mq,
        {.start = [] {}, .stop = [] {}},
        {.start = [] {}, .stop = [] {}}, callback);
    assert(context.Start());
    assert(context.IsRunning());
    context.Stop();
    context.Stop();

    const std::vector<std::string> expected{
        "Logger:started", "Monitor:started", "ODB:started", "Redis:started",
        "MQ:started",     "Executor:started", "Executor:stopped", "MQ:stopped",
        "Redis:stopped",  "ODB:stopped",      "Monitor:stopped",  "Logger:stopped"};
    assert(order == expected);
    assert(odb.starts == 1 && odb.stops == 1);
    assert(mq.starts == 1 && mq.stops == 1);
    assert(!context.IsRunning());
}

void TestContextStartupFailureRollsBack()
{
    FakeOdbPool odb;
    FakeMq mq;
    mq.fail_start = true;
    std::vector<std::string> order;

    ApplicationContext context(
        ContextConfig(), odb, mq,
        {.start = [] {}, .stop = [] {}},
        {.start = [] {}, .stop = [] {}},
        [&](LifecycleStage stage, LifecycleEvent event) {
            order.push_back(oj::runtime::ToString(stage) + ":" + oj::runtime::ToString(event));
        });

    assert(!context.Start());
    assert(context.LastStartError() != nullptr);
    assert(context.FailedStage() == LifecycleStage::Mq);
    context.Stop();

    const std::vector<std::string> expected{
        "Logger:started", "Monitor:started", "ODB:started", "Redis:started",
        "Redis:stopped",  "ODB:stopped",      "Monitor:stopped", "Logger:stopped"};
    assert(order == expected);
    assert(odb.starts == 1 && odb.stops == 1);
    assert(mq.starts == 1 && mq.stops == 0);
}

void TestContextsOwnIndependentRuntimeState()
{
    FakeOdbPool odb_one;
    FakeOdbPool odb_two;
    FakeMq mq_one;
    FakeMq mq_two;
    ApplicationContext first(ContextConfig(), odb_one, mq_one, {}, {});
    ApplicationContext second(ContextConfig(), odb_two, mq_two, {}, {});

    assert(&first.Monitor() != &second.Monitor());
    assert(&first.Executor() != &second.Executor());
}

} // namespace

int main()
{
    TestExecutorSaturationAndFifoDrain();
    TestExecutorCancelPending();
    TestExecutorExceptionIsolation();
    TestContextLifecycleOrderAndRepeatedStop();
    TestContextStartupFailureRollsBack();
    TestContextsOwnIndependentRuntimeState();
}
