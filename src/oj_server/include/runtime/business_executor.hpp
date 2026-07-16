#pragma once

#include <cstddef>
#include <condition_variable>
#include <exception>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

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

    using Task = std::function<void()>;
    using ExceptionHandler = std::function<void(std::exception_ptr)>;

    explicit BusinessExecutor(Config config, ExceptionHandler exception_handler = {});
    ~BusinessExecutor();

    BusinessExecutor(const BusinessExecutor&) = delete;
    BusinessExecutor& operator=(const BusinessExecutor&) = delete;

    bool Start();
    SubmitResult Submit(Task task);
    std::size_t PendingTaskCount() const;

private:
    void WorkerLoop();

    Config config_;
    ExceptionHandler exception_handler_;
    mutable std::mutex mutex_;
    std::mutex lifecycle_mutex_;
    std::condition_variable work_available_;
    std::queue<Task> tasks_;
    std::vector<std::thread> workers_;
    StopPolicy active_stop_policy_ = StopPolicy::Drain;
};

} // namespace oj::runtime
