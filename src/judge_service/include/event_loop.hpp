#pragma once

#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <coroutine>
#include <ctime>
#include <exception>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "../../comm/comm.hpp"
#include "../../comm/proto/common.pb.h"

namespace oj::judge
{
    struct DockerTaskResult
    {
        std::string status;
        int exit_code = -1;
        bool timed_out = false;
        bool killed = false;
        int64_t time_used_ms = 0;
        uint64_t memory_used_bytes = 0;
        std::string stdout_str;
        std::string stderr_str;
    };

    class CoTask
    {
    public:
        struct promise_type
        {
            using CallBack = std::function<void(const oj::common::JudgeResult&)>;
            using IoWork = std::function<DockerTaskResult()>;
            using IoSubmit = std::function<bool(IoWork, int)>;

            std::exception_ptr exception;
            CallBack cb;
            IoSubmit submit_io;
            DockerTaskResult result;
            std::shared_ptr<oj::common::JudgeResult> request;

            CoTask get_return_object() { return CoTask{handle_type::from_promise(*this)}; }
            std::suspend_always initial_suspend() noexcept { return {}; }
            std::suspend_always final_suspend() noexcept { return {}; }
            void return_value(std::shared_ptr<oj::common::JudgeResult> value) { request = std::move(value); }
            void unhandled_exception()
            {
                exception = std::current_exception();
                request = std::make_shared<oj::common::JudgeResult>();
                request->set_status(oj::common::SUBMISSION_STATUS_SYSTEM_ERROR);
                request->set_completed_at(std::time(nullptr));
            }
        };

        using handle_type = std::coroutine_handle<promise_type>;

        CoTask() = default;
        explicit CoTask(handle_type handle) : _handle(handle) {}
        CoTask(const CoTask&) = delete;
        CoTask& operator=(const CoTask&) = delete;
        CoTask(CoTask&& other) noexcept : _handle(std::exchange(other._handle, nullptr)) {}
        CoTask& operator=(CoTask&& other) noexcept
        {
            if (this != &other) {
                if (_handle) _handle.destroy();
                _handle = std::exchange(other._handle, nullptr);
            }
            return *this;
        }
        ~CoTask() { if (_handle) _handle.destroy(); }

        void Resume() { if (_handle && !_handle.done()) _handle.resume(); }
        bool Done() const { return !_handle || _handle.done(); }
        handle_type GetHandle() const { return _handle; }

    private:
        handle_type _handle = nullptr;
    };

    struct DockerTaskAwaitable
    {
        int timeout_ms = 5000;
        CoTask::promise_type::IoWork work;
        CoTask::handle_type handle = nullptr;

        bool await_ready() const noexcept { return false; }
        bool await_suspend(CoTask::handle_type handle)
        {
            this->handle = handle;
            if (!handle.promise().submit_io || !work) {
                handle.promise().result = {"SYSTEM_ERROR", -1, false, false, 0, 0, {},
                                           "Docker I/O executor is unavailable"};
                return false;
            }
            if (handle.promise().submit_io(std::move(work), std::max(1, timeout_ms))) return true;
            handle.promise().result = {"CANCELLED", -1, false, true, 0, 0, {},
                                       "Judge event loop stopped"};
            return false;
        }
        DockerTaskResult await_resume() noexcept { return std::move(handle.promise().result); }
    };

    class JudgeEventLoop
    {
        enum class PendingState { Ready, Running, Waiting };

        struct PendingEntry
        {
            CoTask task;
            PendingState state = PendingState::Ready;
            uint64_t generation = 0;
            std::chrono::steady_clock::time_point deadline;
            std::chrono::steady_clock::time_point io_deadline;
            bool io_in_flight = false;
            bool cancel_requested = false;
        };

        struct IoJob
        {
            std::string task_id;
            uint64_t generation = 0;
            std::chrono::steady_clock::time_point deadline;
            CoTask::promise_type::IoWork work;
        };

    public:
        JudgeEventLoop() = default;
        ~JudgeEventLoop() { Stop(); }

        void Run(int thread_nums)
        {
            bool expected = false;
            if (!_running.compare_exchange_strong(expected, true, std::memory_order_acq_rel)) return;
            _accepting_io.store(true, std::memory_order_release);
            _stop_owners.store(false, std::memory_order_release);
            const int count = std::max(1, thread_nums);
            for (int i = 0; i < count; ++i) _owner_threads.emplace_back(&JudgeEventLoop::OwnerWork, this);
            for (int i = 0; i < count; ++i) _io_threads.emplace_back(&JudgeEventLoop::IoWork, this);
        }

        void Stop()
        {
            _accepting_io.store(false, std::memory_order_release);
            if (!_running.exchange(false, std::memory_order_acq_rel)) return;
            {
                std::lock_guard<std::mutex> lock(_io_mutex);
                std::queue<IoJob> abandoned;
                _io_queue.swap(abandoned);
            }
            _io_cv.notify_all();
            for (auto& thread : _io_threads) if (thread.joinable()) thread.join();
            _io_threads.clear();

            {
                std::lock_guard<std::mutex> lock(_mutex);
                for (auto& [task_id, pending] : _pendings) {
                    pending.io_in_flight = false;
                    pending.cancel_requested = true;
                    if (pending.state != PendingState::Running) {
                        pending.state = PendingState::Ready;
                        _ready.push(task_id);
                    }
                }
            }
            _stop_owners.store(true, std::memory_order_release);
            _cv.notify_all();
            for (auto& thread : _owner_threads) if (thread.joinable()) thread.join();
            _owner_threads.clear();
        }

        std::string PushNewTask(CoTask&& task, int timeout_ms,
                                CoTask::promise_type::CallBack callback)
        {
            std::lock_guard<std::mutex> lock(_mutex);
            if (!_running.load(std::memory_order_acquire) ||
                !_accepting_io.load(std::memory_order_acquire) || !task.GetHandle()) return {};
            std::string task_id = oj::util::StringUtil::GetUniqueName();
            auto [it, inserted] = _pendings.try_emplace(task_id);
            if (!inserted) return {};
            it->second.task = std::move(task);
            it->second.deadline = std::chrono::steady_clock::now() +
                                  std::chrono::milliseconds(std::max(1, timeout_ms));
            it->second.task.GetHandle().promise().cb = std::move(callback);
            it->second.task.GetHandle().promise().submit_io =
                [this, task_id](CoTask::promise_type::IoWork work, int timeout_ms) {
                    return SubmitIo(task_id, std::move(work), timeout_ms);
                };
            _ready.push(task_id);
            _cv.notify_one();
            return task_id;
        }

        std::string PushNewTask(CoTask&& task) { return PushNewTask(std::move(task), 30000, {}); }

        size_t PendingCount() const
        {
            std::lock_guard<std::mutex> lock(_mutex);
            return _pendings.size();
        }

    private:
        bool SubmitIo(const std::string& task_id, CoTask::promise_type::IoWork work,
                      int timeout_ms)
        {
            std::scoped_lock lock(_mutex, _io_mutex);
            auto it = _pendings.find(task_id);
            if (!_accepting_io.load(std::memory_order_acquire) || it == _pendings.end() ||
                it->second.state != PendingState::Running || it->second.cancel_requested) return false;
            const auto now = std::chrono::steady_clock::now();
            if (now >= it->second.deadline) {
                it->second.cancel_requested = true;
                return false;
            }
            it->second.state = PendingState::Waiting;
            it->second.io_in_flight = true;
            it->second.io_deadline = std::min(
                it->second.deadline, now + std::chrono::milliseconds(std::max(1, timeout_ms)));
            const uint64_t generation = ++it->second.generation;
            _io_queue.push({task_id, generation, it->second.io_deadline, std::move(work)});
            _io_cv.notify_one();
            return true;
        }

        void CompleteIo(const std::string& task_id, uint64_t generation, DockerTaskResult result)
        {
            std::lock_guard<std::mutex> lock(_mutex);
            auto it = _pendings.find(task_id);
            if (it == _pendings.end() || it->second.generation != generation) return;
            it->second.io_in_flight = false;
            if (it->second.cancel_requested || !_running.load(std::memory_order_acquire)) {
                it->second.cancel_requested = true;
                if (it->second.state != PendingState::Running) {
                    it->second.state = PendingState::Ready;
                    _ready.push(task_id);
                    _cv.notify_one();
                }
                return;
            }
            if (std::chrono::steady_clock::now() >= it->second.io_deadline) {
                result.status = "TIME_LIMIT";
                result.timed_out = true;
                result.killed = result.killed || result.exit_code < 0;
                if (result.stderr_str.empty()) result.stderr_str = "Docker I/O deadline exceeded";
            }
            it->second.task.GetHandle().promise().result = std::move(result);
            it->second.state = PendingState::Ready;
            _ready.push(task_id);
            _cv.notify_one();
        }

        void IoWork()
        {
            while (true) {
                IoJob job;
                {
                    std::unique_lock<std::mutex> lock(_io_mutex);
                    _io_cv.wait(lock, [this] { return !_running.load() || !_io_queue.empty(); });
                    if (!_running.load()) return;
                    job = std::move(_io_queue.front());
                    _io_queue.pop();
                }
                DockerTaskResult result;
                if (std::chrono::steady_clock::now() >= job.deadline) {
                    result = {"TIME_LIMIT", -1, true, false, 0, 0, {},
                              "Docker I/O deadline exceeded before execution"};
                } else {
                    try {
                        result = job.work();
                    } catch (const std::exception& e) {
                        result.status = "SYSTEM_ERROR";
                        result.stderr_str = e.what();
                    } catch (...) {
                        result.status = "SYSTEM_ERROR";
                        result.stderr_str = "Unknown Docker executor failure";
                    }
                }
                CompleteIo(job.task_id, job.generation, std::move(result));
            }
        }

        void OwnerWork()
        {
            while (true) {
                std::string task_id;
                CoTask* task = nullptr;
                bool stopping = false;
                {
                    std::unique_lock<std::mutex> lock(_mutex);
                    if (_pendings.empty()) {
                        _cv.wait(lock, [this] {
                            return _stop_owners.load() || !_ready.empty() || !_pendings.empty();
                        });
                    } else {
                        _cv.wait_for(lock, std::chrono::milliseconds(5),
                                     [this] { return _stop_owners.load() || !_ready.empty(); });
                    }
                    const auto now = std::chrono::steady_clock::now();
                    for (auto& [id, pending] : _pendings) {
                        if (!pending.cancel_requested && now >= pending.deadline) {
                            pending.cancel_requested = true;
                            if (pending.state != PendingState::Running) {
                                pending.state = PendingState::Ready;
                                _ready.push(id);
                            }
                        }
                    }
                    if (_ready.empty()) {
                        if (_stop_owners.load()) return;
                        continue;
                    }
                    task_id = std::move(_ready.front());
                    _ready.pop();
                    auto it = _pendings.find(task_id);
                    if (it == _pendings.end() || it->second.state != PendingState::Ready) continue;
                    it->second.state = PendingState::Running;
                    task = &it->second.task;
                    stopping = !_running.load() || it->second.cancel_requested;
                }

                if (!stopping) task->Resume();

                std::unique_lock<std::mutex> lock(_mutex);
                auto it = _pendings.find(task_id);
                if (it == _pendings.end()) continue;
                if (stopping || it->second.cancel_requested) {
                    auto callback = std::move(it->second.task.GetHandle().promise().cb);
                    lock.unlock();
                    if (callback) {
                        oj::common::JudgeResult cancelled;
                        cancelled.set_status(oj::common::SUBMISSION_STATUS_CANCELLED);
                        cancelled.set_completed_at(std::time(nullptr));
                        try { callback(cancelled); } catch (...) {}
                    }
                    lock.lock();
                    it = _pendings.find(task_id);
                    if (it != _pendings.end()) {
                        if (it->second.io_in_flight) it->second.state = PendingState::Waiting;
                        else _pendings.erase(it);
                    }
                    continue;
                }
                if (!it->second.task.Done()) continue;
                auto callback = std::move(it->second.task.GetHandle().promise().cb);
                auto result = it->second.task.GetHandle().promise().request;
                if (!result) {
                    result = std::make_shared<oj::common::JudgeResult>();
                    result->set_status(oj::common::SUBMISSION_STATUS_SYSTEM_ERROR);
                }
                lock.unlock();
                if (callback) {
                    try { callback(*result); } catch (...) {}
                }
                lock.lock();
                _pendings.erase(task_id); // owner worker is the only coroutine destroyer
            }
        }

        mutable std::mutex _mutex;
        std::condition_variable _cv;
        std::map<std::string, PendingEntry> _pendings;
        std::queue<std::string> _ready;

        std::mutex _io_mutex;
        std::condition_variable _io_cv;
        std::queue<IoJob> _io_queue;
        std::atomic<bool> _running{false};
        std::atomic<bool> _accepting_io{false};
        std::atomic<bool> _stop_owners{false};
        std::vector<std::thread> _owner_threads;
        std::vector<std::thread> _io_threads;
    };
}
