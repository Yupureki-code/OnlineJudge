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
    if (!started_ || stopping_ || stopped_) {
        rejected_stopped_count_.fetch_add(1, std::memory_order_relaxed);
        return SubmitResult::Stopped;
    }
    if (tasks_.size() >= config_.queue_capacity) {
        rejected_queue_full_count_.fetch_add(1, std::memory_order_relaxed);
        return SubmitResult::QueueFull;
    }
    tasks_.push({std::move(task), std::chrono::steady_clock::now()});
    accepted_count_.fetch_add(1, std::memory_order_relaxed);
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
    std::queue<QueuedTask> cancelled;
    {
        std::lock_guard lock(mutex_);
        if (stopped_) return;
        if (!started_) {
            stopped_ = true;
            return;
        }
        stopping_ = true;
        active_stop_policy_ = policy;
        if (policy == StopPolicy::CancelPending) {
            cancelled_pending_count_.fetch_add(tasks_.size(), std::memory_order_relaxed);
            tasks_.swap(cancelled);
        }
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

BusinessExecutor::Snapshot BusinessExecutor::GetSnapshot() const
{
    Snapshot snapshot;
    snapshot.worker_count = config_.worker_count;
    snapshot.queue_capacity = config_.queue_capacity;
    {
        std::lock_guard lock(mutex_);
        snapshot.queue_depth = tasks_.size();
    }
    snapshot.active_workers = active_workers_.load(std::memory_order_relaxed);
    snapshot.accepted_count = accepted_count_.load(std::memory_order_relaxed);
    snapshot.rejected_queue_full_count = rejected_queue_full_count_.load(std::memory_order_relaxed);
    snapshot.rejected_stopped_count = rejected_stopped_count_.load(std::memory_order_relaxed);
    snapshot.completed_count = completed_count_.load(std::memory_order_relaxed);
    snapshot.cancelled_pending_count = cancelled_pending_count_.load(std::memory_order_relaxed);
    snapshot.queue_wait_count = queue_wait_count_.load(std::memory_order_relaxed);
    snapshot.queue_wait_total_us = queue_wait_total_us_.load(std::memory_order_relaxed);
    snapshot.queue_wait_max_us = queue_wait_max_us_.load(std::memory_order_relaxed);
    snapshot.execution_count = execution_count_.load(std::memory_order_relaxed);
    snapshot.execution_total_us = execution_total_us_.load(std::memory_order_relaxed);
    snapshot.execution_max_us = execution_max_us_.load(std::memory_order_relaxed);
    return snapshot;
}

void BusinessExecutor::UpdateMaximum(std::atomic<std::uint64_t>& maximum, std::uint64_t value)
{
    auto current = maximum.load(std::memory_order_relaxed);
    while (current < value &&
           !maximum.compare_exchange_weak(current, value, std::memory_order_relaxed)) {
    }
}

void BusinessExecutor::WorkerLoop()
{
    for (;;) 
    {
        QueuedTask queued;
        {
            std::unique_lock lock(mutex_);
            work_available_.wait(lock, [this] { return stopping_ || !tasks_.empty(); });
            if (stopping_ && (active_stop_policy_ == StopPolicy::CancelPending || tasks_.empty()))
                return;
            queued = std::move(tasks_.front());
            tasks_.pop();
        }

        const auto started_at = std::chrono::steady_clock::now();
        const auto queue_wait_us = static_cast<std::uint64_t>(
            std::chrono::duration_cast<std::chrono::microseconds>(started_at - queued.enqueued_at).count());
        queue_wait_count_.fetch_add(1, std::memory_order_relaxed);
        queue_wait_total_us_.fetch_add(queue_wait_us, std::memory_order_relaxed);
        UpdateMaximum(queue_wait_max_us_, queue_wait_us);
        active_workers_.fetch_add(1, std::memory_order_relaxed);

        try 
        {
            if (queued.task)
                queued.task();
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
        const auto execution_us = static_cast<std::uint64_t>(
            std::chrono::duration_cast<std::chrono::microseconds>(
                std::chrono::steady_clock::now() - started_at).count());
        execution_count_.fetch_add(1, std::memory_order_relaxed);
        execution_total_us_.fetch_add(execution_us, std::memory_order_relaxed);
        UpdateMaximum(execution_max_us_, execution_us);
        completed_count_.fetch_add(1, std::memory_order_relaxed);
        active_workers_.fetch_sub(1, std::memory_order_relaxed);
    }
}

} // namespace oj::runtime
