#pragma once

#include <atomic>
#include <chrono>
#include <cstddef>
#include <condition_variable>
#include <exception>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>
#include <cstdint>

namespace oj::runtime
{

enum class SubmitResult
{
    Accepted,
    QueueFull,
    Stopped
};

enum class StopPolicy
{
    Drain,
    CancelPending
};

class BusinessExecutor
{
public:
    struct Config
    {
        std::size_t worker_count = 4;
        std::size_t queue_capacity = 256;
        StopPolicy stop_policy = StopPolicy::Drain;
    };

    struct Snapshot
    {
        std::uint64_t worker_count = 0;
        std::uint64_t queue_capacity = 0;
        std::uint64_t queue_depth = 0;
        std::uint64_t active_workers = 0;
        std::uint64_t accepted_count = 0;
        std::uint64_t rejected_queue_full_count = 0;
        std::uint64_t rejected_stopped_count = 0;
        std::uint64_t completed_count = 0;
        std::uint64_t cancelled_pending_count = 0;
        std::uint64_t queue_wait_count = 0;
        std::uint64_t queue_wait_total_us = 0;
        std::uint64_t queue_wait_max_us = 0;
        std::uint64_t execution_count = 0;
        std::uint64_t execution_total_us = 0;
        std::uint64_t execution_max_us = 0;
    };

    using Task = std::function<void()>;
    using ExceptionHandler = std::function<void(std::exception_ptr)>;

    explicit BusinessExecutor(Config config, ExceptionHandler exception_handler = {});
    ~BusinessExecutor();

    BusinessExecutor(const BusinessExecutor&) = delete;
    BusinessExecutor& operator=(const BusinessExecutor&) = delete;

    bool Start();
    SubmitResult Submit(Task task);
    void Stop();
    void Stop(StopPolicy policy);
    bool IsAccepting() const;
    std::size_t PendingTaskCount() const;
    Snapshot GetSnapshot() const;

private:
    struct QueuedTask
    {
        Task task;
        std::chrono::steady_clock::time_point enqueued_at;
    };

    static void UpdateMaximum(std::atomic<std::uint64_t>& maximum, std::uint64_t value);
    void WorkerLoop();

    Config config_;
    ExceptionHandler exception_handler_;
    mutable std::mutex mutex_;
    std::mutex lifecycle_mutex_;
    std::condition_variable work_available_;
    std::queue<QueuedTask> tasks_;
    std::vector<std::thread> workers_;
    bool started_ = false;
    bool stopping_ = false;
    bool stopped_ = false;
    StopPolicy active_stop_policy_ = StopPolicy::Drain;
    std::atomic<std::uint64_t> active_workers_{0};
    std::atomic<std::uint64_t> accepted_count_{0};
    std::atomic<std::uint64_t> rejected_queue_full_count_{0};
    std::atomic<std::uint64_t> rejected_stopped_count_{0};
    std::atomic<std::uint64_t> completed_count_{0};
    std::atomic<std::uint64_t> cancelled_pending_count_{0};
    std::atomic<std::uint64_t> queue_wait_count_{0};
    std::atomic<std::uint64_t> queue_wait_total_us_{0};
    std::atomic<std::uint64_t> queue_wait_max_us_{0};
    std::atomic<std::uint64_t> execution_count_{0};
    std::atomic<std::uint64_t> execution_total_us_{0};
    std::atomic<std::uint64_t> execution_max_us_{0};
};

} // namespace oj::runtime
