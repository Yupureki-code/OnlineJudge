#pragma once
#include <coroutine>
#include <exception>
#include <optional>
#include <memory>
#include <utility>
#include <exception>

namespace ns_async {

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
template<typename T>
class Task {
public:
    struct promise_type {
        Task get_return_object() {
            return Task{handle_type::from_promise(*this)};
        }
        std::suspend_always initial_suspend() noexcept { return {}; }
        std::suspend_always final_suspend() noexcept { return {}; }
        void return_value(T value) { _result = std::move(value); }
        void unhandled_exception() { _exception = std::current_exception(); }

        std::optional<T> _result;
        std::exception_ptr _exception;
    };

    using handle_type = std::coroutine_handle<promise_type>;

    Task() = default;
    explicit Task(handle_type h) : _handle(h) {}
    Task(Task&& other) noexcept : _handle(std::exchange(other._handle, {})) {}
    Task& operator=(Task&& other) noexcept {
        if (this != &other) {
            if (_handle) _handle.destroy();
            _handle = std::exchange(other._handle, {});
        }
        return *this;
    }
    Task(const Task&) = delete;
    Task& operator=(const Task&) = delete;
    ~Task() { if (_handle) _handle.destroy(); }

    /// 恢复协程
    void resume() { if (_handle && !_handle.done()) _handle.resume(); }

    /// 协程是否完成
    bool done() const { return !_handle || _handle.done(); }

    /// 获取结果（协程完成后调用）
    T GetResult() {
        if (_handle->_exception) std::rethrow_exception(_handle->_exception);
        return std::move(*_handle->_result);
    }

    /// 获取协程句柄
    handle_type GetHandle() const { return _handle; }

private:
    handle_type _handle;
};

/// Task<void> 特化
template<>
class Task<void> {
public:
    struct promise_type {
        std::coroutine_handle<> continuation_ = nullptr;
        Task get_return_object() { return Task{handle_type::from_promise(*this)}; }
        std::suspend_always initial_suspend() noexcept { return {}; }
        std::suspend_always final_suspend() noexcept { return {}; }
        void return_void() {}
        void unhandled_exception() { _exception = std::current_exception(); }
        std::exception_ptr _exception;
    };

    using handle_type = std::coroutine_handle<promise_type>;

    Task() = default;
    explicit Task(handle_type h) : _handle(h) {}
    Task(Task&& other) noexcept : _handle(std::exchange(other._handle, {})) {}
    Task& operator=(Task&& other) noexcept {
        if (this != &other) {
            if (_handle) _handle.destroy();
            _handle = std::exchange(other._handle, {});
        }
        return *this;
    }
    Task(const Task&) = delete;
    Task& operator=(const Task&) = delete;
    ~Task() { if (_handle) _handle.destroy(); }

    void resume() { if (_handle && !_handle.done()) _handle.resume(); }
    bool done() const { return !_handle || _handle.done(); }
    void GetResult() { if (_handle->_exception) std::rethrow_exception(_handle->_exception); }
    handle_type GetHandle() const { return _handle; }
     auto operator co_await() noexcept {
        struct Awaiter {
            std::coroutine_handle<promise_type> child_;
            // ① 快速路径：子协程已完成？
            [[nodiscard]] bool await_ready() const noexcept {
                return child_.done();
            }
            // ② 核心：建立父子关系，对称转移到子协程
            [[nodiscard]] std::coroutine_handle<>
            await_suspend(std::coroutine_handle<> caller) noexcept {
                // 设置 continuation：子协程结束时知道该唤醒谁
                child_.promise().continuation_ = caller;
                return child_;  // ← 对称转移
            }
            // ③ 取回结果
            void await_resume() {}
        };
        return Awaiter{_handle};
    }
private:
    handle_type _handle;
};

} // namespace ns_async