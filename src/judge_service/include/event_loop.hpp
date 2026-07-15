#pragma once
#include <chrono>
#include <condition_variable>
#include <event2/event.h>
#include <event2/thread.h>
#include <queue>
#include <string>
#include <thread>
#include <functional>
#include "../../comm/comm.hpp"
#include <unordered_map>
#include "../../comm/proto/judge_service.pb.h"
#include <coroutine>
#include <exception>
#include <functional>


namespace oj_judge 
{
    /// JudgeEventLoop — libevent 事件循环封装
    ///
    /// 职责：
    /// 1. 运行 libevent 事件循环（处理 MQ socket 事件、定时器事件）
    /// 2. 被 brpc DockerWorkDone handler 线程通过 event_active 唤醒
    /// 3. 检查 PendingTaskManager 中已完成/超时的任务 → resume 协程
    struct DockerTaskResult 
    {
        std::string status;       // OK / COMPILE_ERROR / MEMORY_LIMIT / SEGV / FPE / TIME_LIMIT / TIMEOUT
        int exit_code = -1;       // 进程退出码
        bool timed_out = false;   // 是否超时
    };
    enum PendingStatus
    {
        Working,
        Blocking,
        Done,
        Finished,
        TimeOut
    };
    /// Task<T> — C++20 协程返回类型
    ///
    /// 用法：
    ///   Task<JudgeResult> JudgeAsync(const JudgeTask& task) {
    ///       co_await DockerTaskAwaitable{...};
    ///       co_return result;
    ///   }
    ///
    ///   auto coro = JudgeAsync(task);
    ///   coro.resume();  // 启动协程，遇到 co_await 时挂起
    ///   // ... 事件循环恢复协程后 ...
    ///   auto result = coro.GetResult();
    class CoTask 
    {
    public:
        struct promise_type 
        {
            using CallBack = std::function<void(const JudgeFinishedRequest& request)>;
            std::string task_id;
            std::coroutine_handle<promise_type> home = nullptr;
            std::exception_ptr exception;
            CallBack cb;
            DockerTaskResult result;
            PendingStatus* status = nullptr;
            std::shared_ptr<JudgeFinishedRequest> request;
            CoTask get_return_object() 
            {
                return CoTask{handle_type::from_promise(*this)};
            }
            std::suspend_always initial_suspend() noexcept { return {}; }
            void return_value(std::shared_ptr<JudgeFinishedRequest> req)
            {
                request = req;
            }
            auto final_suspend() noexcept 
            {
                struct FinalAwaiter 
                {
                    const oj_judge::JudgeFinishedRequest& request;
                    CallBack cb;
                    bool await_ready() noexcept { return false; }
                    std::coroutine_handle<> await_suspend(std::coroutine_handle<promise_type> h) noexcept 
                    {
                        auto& promise = h.promise();
                        if (promise.cb) promise.cb(request);
                        if (promise.status) *promise.status = Finished;
                        if (promise.home) 
                        {
                            return promise.home;
                        }
                        return std::noop_coroutine();
                    }
                    void await_resume() noexcept {}
                };
                return FinalAwaiter{*request};
            }
            void unhandled_exception() { exception = std::current_exception(); }
        };

        using handle_type = std::coroutine_handle<promise_type>;

        CoTask() : _handle(nullptr) {}
        CoTask(std::coroutine_handle<promise_type> h) {_handle = h;}

        // Move-only — prevent double-free of coroutine handle
        CoTask(const CoTask&) = delete;
        CoTask& operator=(const CoTask&) = delete;
        CoTask(CoTask&& other) noexcept : _handle(other._handle) { other._handle = nullptr; }
        CoTask& operator=(CoTask&& other) noexcept {
            if (this != &other) { if (_handle) _handle.destroy(); _handle = other._handle; other._handle = nullptr; }
            return *this;
        }
        ~CoTask() { if (_handle) _handle.destroy(); }
        /// 恢复协程
        void resume() { if (_handle && !_handle.done()) _handle.resume(); }
        /// 协程是否完成
        bool done() const { return !_handle || _handle.done(); }
        /// 获取结果（协程完成后调用）
        /// 获取协程句柄
        handle_type GetHandle() const { return _handle; }

    private:
        handle_type _handle;
    };

    struct LoopAwaitable 
    {
        std::coroutine_handle<CoTask::promise_type> parent_handle;
        PendingStatus* status;
        bool await_ready() const { return false; }  // 总是挂起
        
        std::coroutine_handle<CoTask::promise_type> await_suspend(std::coroutine_handle<CoTask::promise_type> h) 
        {
            parent_handle.promise().home = h;
            parent_handle.promise().status = status;
            return parent_handle;
        }

        void await_resume() 
        {
            return;
        }
    };
    struct DockerTaskAwaitable 
    {
        std::string container_id;    // Docker 容器 ID
        std::string script;           // 容器内执行的脚本命令
        std::string task_id;
        int timeout_ms;               // 超时时间
        std::coroutine_handle<CoTask::promise_type> home;
        /// 启动容器的回调（由外部注入，实际调用 Docker API）
        /// 参数: container_id, script, task_id
        std::function<void(const std::string&, const std::string&, const std::string&)> start_container;

        bool await_ready() const { return false; }  // 总是挂起

        std::coroutine_handle<CoTask::promise_type> await_suspend(std::coroutine_handle<CoTask::promise_type> h) 
        {
            if (start_container) 
            {
                start_container(container_id, script, task_id);
            }
            return home;
        }

        DockerTaskResult await_resume() 
        {
            return {};
        }
    };
    struct PendingEntry 
    {
        std::string task_id;
        CoTask task;
        int64_t start_time;
        int timeout_ms;
        PendingStatus status;
    };
    class JudgeEventLoop 
    {
    private:
        CoTask Work()
        {
            for(;;)
            {
                // 非阻塞处理一轮事件
                thread_local std::string task_id;
                thread_local PendingEntry* pending;
                {
                    std::unique_lock<std::mutex> lock(_mutex);
                    _cv.wait(lock, [this]
                    {
                        return !_task_queue.empty();
                    });
                    task_id = _task_queue.front();
                    _task_queue.pop();
                    auto it = _pendings.find(task_id);
                    if(it == _pendings.end())
                        continue;
                    pending = &it->second;
                    if(pending->status != Done)
                        continue;
                    pending->status = Working;
                }
                co_await LoopAwaitable{pending->task.GetHandle(),&pending->status};
                // Coroutine finished — clean up from _pendings
                {
                    std::unique_lock<std::mutex> lock(_mutex);
                    _pendings.erase(task_id);
                }
            }
        }
        void WorkHelper()
        {
            auto task = Work();
            task.resume();
        }
        void CheckTimeOut()
        {
            for(;;)
            {
                std::unique_lock<std::mutex> lock(_mutex);
                _cv.wait_for(lock, std::chrono::milliseconds(30), [this]
                {
                    return !_pendings.empty();
                });
                // Collect keys to erase (avoid iterator invalidation)
                std::vector<std::string> to_erase;
                for(auto & it : _pendings)
                {
                    auto& pending = it.second;
                    // Only erase timed-out tasks — Finished tasks are cleaned up by Work()
                    if(pending.status == Blocking && oj_util::TimeUtil::GetTimeStamp() - pending.start_time > pending.timeout_ms)
                    {
                        to_erase.push_back(it.first);
                    }
                }
                for(auto& key : to_erase) {
                    _pendings.erase(key);  // ~CoTask destroys coroutine handle
                }
            }
        }
    public:
        JudgeEventLoop() 
        { }
        /// 启动事件循环（阻塞当前线程）
        void Run(int thread_nums) 
        {
            for(int i = 1;i<=thread_nums;i++)
            {
                std::thread(std::bind(&JudgeEventLoop::WorkHelper,this)).detach();
            }
            std::thread(std::bind(&JudgeEventLoop::CheckTimeOut,this)).detach();
        }
        std::string PushNewTask(CoTask&& task)
        {
            std::unique_lock<std::mutex> lock(_mutex);
            std::string task_id = oj_util::StringUtil::GetUniqueName();
            _task_queue.push(task_id);
            _cv.notify_one();
            PendingEntry pending;
            pending.task_id = task_id;
            pending.start_time = oj_util::TimeUtil::GetTimeStamp();
            pending.timeout_ms = 1000;
            pending.task = std::move(task);
            pending.status = Done; 
            _pendings[task_id] = std::move(pending);
            return task_id;
        }
        void PushDockerDoneTask(const std::string& task_id,const DockerTaskResult& result)
        {
            std::unique_lock<std::mutex> lock(_mutex);
            auto it = _pendings.find(task_id);
            if(it == _pendings.end() || it->second.status != Blocking)
                return;
            it->second.status = Done;
            it->second.task.GetHandle().promise().result = result;
            _task_queue.push(task_id);
            _cv.notify_one();
        }
    private:
        std::unordered_map<std::string, PendingEntry> _pendings;
        std::queue<std::string> _task_queue;
        std::mutex _mutex;
        std::condition_variable _cv;
    };
}