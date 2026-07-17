#include "../../include/runtime/business_executor.hpp"

#include <stdexcept>
#include <utility>

namespace oj::runtime
{

BusinessExecutor::BusinessExecutor(Config config, ExceptionHandler exception_handler)
    : config_(config), exception_handler_(std::move(exception_handler))
{
    if (config_.worker_count == 0)
        throw std::invalid_argument("business executor requires at least one worker");
    if (config_.queue_capacity == 0)
        throw std::invalid_argument("business executor requires a non-empty queue");
}

BusinessExecutor::~BusinessExecutor()
{
    Stop(config_.stop_policy);
}

bool BusinessExecutor::Start()
{
    std::lock_guard lifecycle_lock(lifecycle_mutex_);
    {
        std::lock_guard lock(mutex_);
        if (started_ && !stopping_) return true;
        if (stopped_) return false;
        started_ = true;
        stopping_ = false;
        active_stop_policy_ = config_.stop_policy;
    }
    try {
        workers_.reserve(config_.worker_count);
        for (std::size_t i = 0; i < config_.worker_count; ++i)
            workers_.emplace_back(&BusinessExecutor::WorkerLoop, this);
    } catch (...) {
        {
            std::lock_guard lock(mutex_);
            stopping_ = true;
            active_stop_policy_ = StopPolicy::CancelPending;
        }
        work_available_.notify_all();
        for (auto& worker : workers_) {
            if (worker.joinable())
                worker.join();
        }
        workers_.clear();
        {
            std::lock_guard lock(mutex_);
            started_ = false;
            stopped_ = true;
        }
        throw;
    }
    return true;
}

SubmitResult BusinessExecutor::Submit(Task task)
{
    std::lock_guard lock(mutex_);
    if (!started_ || stopping_ || stopped_)
        return SubmitResult::Stopped;
    if (tasks_.size() >= config_.queue_capacity)
        return SubmitResult::QueueFull;
    tasks_.push(std::move(task));
    work_available_.notify_one();
    return SubmitResult::Accepted;
}

void BusinessExecutor::Stop()
{
    Stop(config_.stop_policy);
}

void BusinessExecutor::Stop(StopPolicy policy)
{
    std::lock_guard lifecycle_lock(lifecycle_mutex_);
    std::queue<Task> cancelled;
    {
        std::lock_guard lock(mutex_);
        if (stopped_) return;
        if (!started_) {
            stopped_ = true;
            return;
        }
        stopping_ = true;
        active_stop_policy_ = policy;
        if (policy == StopPolicy::CancelPending)
            tasks_.swap(cancelled);
    }
    // Task destructors may run RPC completions that re-enter the executor.
    while (!cancelled.empty()) cancelled.pop();
    work_available_.notify_all();
    for (auto& worker : workers_) {
        if (worker.joinable()) worker.join();
    }
    workers_.clear();
    std::lock_guard lock(mutex_);
    started_ = false;
    stopped_ = true;
}

bool BusinessExecutor::IsAccepting() const
{
    std::lock_guard lock(mutex_);
    return started_ && !stopping_ && !stopped_;
}

std::size_t BusinessExecutor::PendingTaskCount() const
{
    std::lock_guard lock(mutex_);
    return tasks_.size();
}

void BusinessExecutor::WorkerLoop()
{
    for (;;) 
    {
        Task task;
        {
            std::unique_lock lock(mutex_);
            work_available_.wait(lock, [this] { return stopping_ || !tasks_.empty(); });
            if (stopping_ && (active_stop_policy_ == StopPolicy::CancelPending || tasks_.empty()))
                return;
            task = std::move(tasks_.front());
            tasks_.pop();
        }

        try 
        {
            if (task)
                task();
        } 
        catch (...) 
        {
            if (exception_handler_) 
            {
                try 
                {
                    exception_handler_(std::current_exception());
                } 
                catch (...) 
                {
                }
            }
        }
    }
}

} // namespace oj::runtime
