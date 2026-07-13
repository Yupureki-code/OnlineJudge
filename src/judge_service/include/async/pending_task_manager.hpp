#pragma once
#include <coroutine>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <mutex>
#include <chrono>
#include <atomic>
#include <sstream>

namespace oj_judge 
{

    /// Docker 任务结果
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
    /// PendingTaskManager — 全局任务表
    ///
    /// 协程挂起时注册任务 ID，brpc DockerWorkDone handler 标记完成，
    /// 事件循环检查已完成任务并 resume 协程。
    ///
    /// 线程安全：brpc handler 线程调用 MarkDone，事件循环线程调用 GetReady，
    /// 通过 mutex 保护内部数据结构。
    class PendingTaskManager 
    {
    public:
        static PendingTaskManager& Instance() 
        {
            static PendingTaskManager instance;
            return instance;
        }

        /// 注册任务（协程挂起时调用）
        /// @param h 协程句柄
        /// @param timeout_ms 超时时间（毫秒）
        /// @return 唯一任务 ID
        std::string Register(std::coroutine_handle<> h, int timeout_ms) 
        {
            std::lock_guard<std::mutex> lock(_mtx);
            std::string task_id = GenerateTaskId();
            auto now = std::chrono::steady_clock::now();
            PendingEntry entry{h, now, timeout_ms, false, {}};
            _pending[task_id] = entry;
            return task_id;
        }

        /// 标记完成（brpc DockerWorkDone handler 收到容器通知时调用）
        /// @param task_id 任务 ID
        /// @param status 容器脚本报告的状态
        /// @param exit_code 进程退出码
        void MarkDone(const std::string& task_id,
                    const std::string& status,
                    int exit_code) {
            std::lock_guard<std::mutex> lock(_mtx);
            auto it = _pending.find(task_id);
            if (it == _pending.end()) return;  // 任务可能已超时被移除
            it->second.done = true;
            it->second.result.status = status;
            it->second.result.exit_code = exit_code;
        }

        /// 获取已完成任务（事件循环调用）
        /// @return 已完成任务的 {task_id, 协程句柄} 列表
        std::vector<std::pair<std::string, std::coroutine_handle<>>> GetReady() 
        {
            std::lock_guard<std::mutex> lock(_mtx);
            std::vector<std::pair<std::string, std::coroutine_handle<>>> ready;
            for (auto it = _pending.begin(); it != _pending.end(); ) 
            {
                if (it->second.done) 
                {
                    ready.push_back({it->first, it->second.handle});
                    it = _pending.erase(it);
                } 
                else 
                {
                    ++it;
                }
            }
            return ready;
        }

        /// 获取任务结果（协程恢复时调用）
        /// @param task_id 任务 ID
        /// @return 任务结果（必须在 GetReady 返回后调用，否则结果已丢失）
        DockerTaskResult GetResult(const std::string& task_id) {
            std::lock_guard<std::mutex> lock(_mtx);
            auto it = _results.find(task_id);
            if (it != _results.end()) {
                auto result = it->second;
                _results.erase(it);
                return result;
            }
            return {};
        }

        /// 超时检查（事件循环定期调用）
        /// @return 超时的任务 ID 列表
        std::vector<std::string> CheckTimeouts() {
            std::lock_guard<std::mutex> lock(_mtx);
            std::vector<std::string> timeouts;
            auto now = std::chrono::steady_clock::now();
            for (auto it = _pending.begin(); it != _pending.end(); ) {
                auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                    now - it->second.start_time).count();
                if (elapsed > it->second.timeout_ms) {
                    // 标记超时
                    it->second.done = true;
                    it->second.result.timed_out = true;
                    it->second.result.status = "TIMEOUT";
                    _results[it->first] = it->second.result;
                    timeouts.push_back(it->first);
                    it = _pending.erase(it);
                } else {
                    ++it;
                }
            }
            return timeouts;
        }

        /// 获取挂起任务数
        size_t PendingCount() const {
            std::lock_guard<std::mutex> lock(_mtx);
            return _pending.size();
        }

    private:
        PendingTaskManager() = default;
        std::string GenerateTaskId() {
            static std::atomic<uint64_t> counter{0};
            auto now = std::chrono::steady_clock::now().time_since_epoch();
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
            std::stringstream ss;
            ss << std::hex << ms << "_" << counter.fetch_add(1);
            return ss.str();
        }

        mutable std::mutex _mtx;
        std::unordered_map<std::string, PendingEntry> _pending;
        std::unordered_map<std::string, DockerTaskResult> _results;  // 已完成但未被取走的结果
    };

} // namespace ns_async