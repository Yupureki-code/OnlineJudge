#pragma once
#include <chrono>
#include <condition_variable>
#include <event2/event.h>
#include <event2/thread.h>
#include <queue>
#include <string>
#include <thread>
#include <functional>
#include "../../../comm/comm.hpp"
#include <coroutine>
#include <unordered_map>
#include "task.hpp"

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
        Done
    };
    struct PendingEntry 
    {
        std::coroutine_handle<> handle;
        int64_t start_time;
        int timeout_ms;
        PendingStatus status;
        DockerTaskResult result;
    };
    using task_t = std::function<ns_async::Task<void>(std::coroutine_handle<> h)>;
    class JudgeEventLoop 
    {
    private:
        inline auto CaptureHandle(std::coroutine_handle<>& out) 
        {
            struct Awaiter 
            {
                std::coroutine_handle<>& _out;
                bool await_ready() noexcept { 
                    return false;  // 必须进入 await_suspend，因为我们想要 h
                }
                // ★ 返回 false → 不挂起，协程立即继续执行
                bool await_suspend(std::coroutine_handle<> h) noexcept 
                {
                    _out = h;      // 捕获！
                    return false;  // ← ★ 关键：不挂起！
                }
                void await_resume() noexcept {}
            };
            return Awaiter{out};
        }
        ns_async::Task<void> Work()
        {
            for(;;)
            {
                // 非阻塞处理一轮事件
                thread_local task_t task;
                {
                    std::unique_lock<std::mutex> lock(_mutex);
                    _cv.wait(lock, [this]
                    {
                        return !_task_queue.empty();
                    });
                    task = _task_queue.front();
                    _task_queue.pop();
                }
                std::coroutine_handle<> handler;
                CaptureHandle(handler);
                co_await task(handler);
            }
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
                for(auto & it : _pendings)
                {
                    auto& pending = it.second;
                    if(pending.status == Blocking && oj_util::TimeUtil::GetTimeStamp() - pending.start_time > pending.timeout_ms)
                    {
                        pending.status = Done;
                        pending.result.timed_out = true;
                        pending.result.status = "TIMEOUT";
                        _pendings.erase(it.first);
                    }
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
                std::thread(std::bind(&JudgeEventLoop::Work,this)).detach();
            }
            std::thread(std::bind(&JudgeEventLoop::CheckTimeOut,this)).detach();
        }
        void PushTask(task_t task)
        {
            _task_queue.push(task);
            _cv.notify_one();
        }
        /// 从其他线程唤醒事件循环（线程安全）

    private:
        std::unordered_map<std::string, PendingEntry> _pendings;
        std::queue<task_t> _task_queue;
        std::mutex _mutex;
        std::condition_variable _cv;
    };
} // namespace ns_async