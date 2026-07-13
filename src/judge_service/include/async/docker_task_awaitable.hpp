#pragma once
#include <coroutine>
#include <string>
#include <functional>
#include "event_loop.hpp"

namespace oj_judge
{
    /// DockerTaskAwaitable — Docker 任务异步等待器
    ///
    /// 协程挂起时：
    ///   1. 生成 task_id → 注册到 PendingTaskManager
    ///   2. 启动容器（容器脚本执行完后 curl DockerWorkDone?id=task_id）
    ///
    /// 容器通知到达时：
    ///   1. brpc DockerWorkDone handler → PendingTaskManager.MarkDone
    ///   2. event_active 唤醒事件循环
    ///   3. 事件循环 → GetReady → resume 协程
    ///   4. await_resume → GetResult 拿结果
    struct LoopAwaitable 
    {
        std::coroutine_handle<> parent_handle;
        bool await_ready() const { return false; }  // 总是挂起
        
        std::coroutine_handle<> await_suspend(std::coroutine_handle<> h) 
        {
            return parent_handle;
        }

        DockerTaskResult await_resume() 
        {
            return {};
        }
    };
    struct DockerTaskAwaitable 
    {
        std::string container_id;    // Docker 容器 ID
        std::string script;           // 容器内执行的脚本命令
        int timeout_ms;               // 超时时间

        /// 启动容器的回调（由外部注入，实际调用 Docker API）
        /// 参数: container_id, script, task_id
        std::function<void(const std::string&, const std::string&, const std::string&)> start_container;

        bool await_ready() const { return false; }  // 总是挂起

        void await_suspend(std::coroutine_handle<> h) 
        {
            // 注册任务到全局任务表
            //_task_id = PendingTaskManager::Instance().Register(h, timeout_ms);
            // 启动容器（容器脚本会 curl DockerWorkDone?id=_task_id）
            if (start_container) 
            {
                start_container(container_id, script, _task_id);
            }
        }

        DockerTaskResult await_resume() 
        {
            //return PendingTaskManager::Instance().GetResult(_task_id);
        }
    private:
        std::string _task_id;
    };
} 