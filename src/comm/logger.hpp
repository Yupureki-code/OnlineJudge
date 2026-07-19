#pragma once

#include <spdlog/async_logger.h>
#include <spdlog/details/thread_pool.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include <condition_variable>
#include <atomic>
#include <chrono>
#include <cstdlib>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <mutex>
#include <string>
#include <system_error>
#include <thread>

namespace oj::logger
{
    namespace detail 
    {

        class FlushTrackingSink final : public spdlog::sinks::sink
        {
        public:
            explicit FlushTrackingSink(spdlog::sink_ptr sink) : sink_(std::move(sink)) {}

            void log(const spdlog::details::log_msg& message) override
            {
                sink_->log(message);
            }

            void flush() override
            {
                try {
                    sink_->flush();
                } catch (...) {
                    MarkFlushed();
                    throw;
                }
                MarkFlushed();
            }

            void MarkFlushed()
            {
                {
                    std::lock_guard<std::mutex> lock(mutex_);
                    ++flush_count_;
                }
                flushed_.notify_all();
            }

            void set_pattern(const std::string& pattern) override
            {
                sink_->set_pattern(pattern);
            }

            void set_formatter(std::unique_ptr<spdlog::formatter> formatter) override
            {
                sink_->set_formatter(std::move(formatter));
            }

            std::uint64_t NextFlush() const
            {
                std::lock_guard<std::mutex> lock(mutex_);
                return flush_count_ + 1;
            }

            void WaitForFlush(std::uint64_t target)
            {
                std::unique_lock<std::mutex> lock(mutex_);
                flushed_.wait(lock, [this, target]() { return flush_count_ >= target; });
            }

        private:
            spdlog::sink_ptr sink_;
            mutable std::mutex mutex_;
            std::condition_variable flushed_;
            std::uint64_t flush_count_ = 0;
        };

        struct LoggerRuntime
        {
            std::shared_ptr<spdlog::logger> logger;
            std::shared_ptr<FlushTrackingSink> sink;
            std::string name;
            std::string file;
            spdlog::level::level_enum level = spdlog::level::info;
            std::atomic<bool> stop_periodic_flush{false};
            std::mutex periodic_mutex;
            std::condition_variable periodic_cv;
            std::thread periodic_thread;
        };

        inline std::chrono::milliseconds FlushInterval()
        {
            constexpr long fallback = 1000;
            const char* configured = std::getenv("OJ_LOG_FLUSH_MS");
            if (configured == nullptr || *configured == '\0')
                return std::chrono::milliseconds(fallback);
            char* end = nullptr;
            const long parsed = std::strtol(configured, &end, 10);
            if (end == configured || *end != '\0' || parsed < 100 || parsed > 60000)
                return std::chrono::milliseconds(fallback);
            return std::chrono::milliseconds(parsed);
        }

        inline void StopPeriodicFlush(const std::shared_ptr<LoggerRuntime>& runtime)
        {
            if (!runtime) return;
            runtime->stop_periodic_flush.store(true, std::memory_order_release);
            runtime->periodic_cv.notify_all();
            if (runtime->periodic_thread.joinable()) runtime->periodic_thread.join();
        }

        inline std::mutex& RuntimeMutex()
        {
            static std::mutex mutex;
            return mutex;
        }

        inline std::shared_ptr<LoggerRuntime>& Runtime()
        {
            static std::shared_ptr<LoggerRuntime> runtime;
            return runtime;
        }

        inline std::shared_ptr<spdlog::details::thread_pool> ThreadPool()
        {
            static auto pool = std::make_shared<spdlog::details::thread_pool>(8192, 1);
            return pool;
        }

        inline std::shared_ptr<spdlog::logger> FallbackLogger()
        {
            static auto logger = []() {
                auto sink = std::make_shared<spdlog::sinks::stderr_color_sink_mt>();
                auto fallback = std::make_shared<spdlog::logger>("fallback", std::move(sink));
                fallback->set_level(spdlog::level::trace);
                fallback->set_pattern("%Y-%m-%d %H:%M:%S.%e [%n] [%-8l] [%s:%#] %v");
                fallback->flush_on(spdlog::level::warn);
                return fallback;
            }();
            return logger;
        }

        inline void Flush(const std::shared_ptr<LoggerRuntime>& runtime)
        {
            if (!runtime)
                return;
            const auto target = runtime->sink->NextFlush();
            runtime->logger->flush();
            runtime->sink->WaitForFlush(target);
        }

    } // namespace detail

    inline std::shared_ptr<spdlog::logger> GetLogger()
    {
        std::lock_guard<std::mutex> lock(detail::RuntimeMutex());
        const auto runtime = detail::Runtime();
        return runtime ? runtime->logger : detail::FallbackLogger();
    }

    inline bool InitLogger(const std::string& logger_name,
                        const std::string& logger_file,
                        spdlog::level::level_enum logger_level)
    {
        if (logger_name.empty())
            return false;

        std::lock_guard<std::mutex> lock(detail::RuntimeMutex());
        auto& current = detail::Runtime();
        if (current && current->name == logger_name && current->file == logger_file &&
            current->level == logger_level) {
            return true;
        }

        try {
            if (!logger_file.empty()) {
                const std::filesystem::path parent = std::filesystem::path(logger_file).parent_path();
                if (!parent.empty()) {
                    std::error_code error;
                    std::filesystem::create_directories(parent, error);
                    if (error)
                        return false;
                }
            }

            spdlog::sink_ptr destination;
            if (logger_file.empty())
                destination = std::make_shared<spdlog::sinks::stderr_color_sink_mt>();
            else
                destination = std::make_shared<spdlog::sinks::basic_file_sink_mt>(logger_file, false);

            auto runtime = std::make_shared<detail::LoggerRuntime>();
            runtime->sink = std::make_shared<detail::FlushTrackingSink>(std::move(destination));
            runtime->logger = std::make_shared<spdlog::async_logger>(
                logger_name, runtime->sink, detail::ThreadPool(), spdlog::async_overflow_policy::block);
            runtime->logger->set_level(logger_level);
            runtime->logger->set_pattern("%Y-%m-%d %H:%M:%S.%e [%n] [%-8l] [%s:%#] %v");
            runtime->logger->flush_on(spdlog::level::warn);
            runtime->name = logger_name;
            runtime->file = logger_file;
            runtime->level = logger_level;

            detail::StopPeriodicFlush(current);
            detail::Flush(current);
            current = std::move(runtime);
            const auto interval = detail::FlushInterval();
            auto* active = current.get();
            current->periodic_thread = std::thread([active, interval] {
                std::unique_lock lock(active->periodic_mutex);
                while (!active->periodic_cv.wait_for(lock, interval, [active] {
                    return active->stop_periodic_flush.load(std::memory_order_acquire);
                })) {
                    lock.unlock();
                    active->logger->flush();
                    lock.lock();
                }
            });
            return true;
        } catch (const std::exception&) {
            return false;
        }
    }

    inline void FlushLogger()
    {
        std::shared_ptr<detail::LoggerRuntime> runtime;
        {
            std::lock_guard<std::mutex> lock(detail::RuntimeMutex());
            runtime = detail::Runtime();
        }
        if (runtime)
            detail::Flush(runtime);
        else
            detail::FallbackLogger()->flush();
    }

    inline void ShutdownLogger()
    {
        std::shared_ptr<detail::LoggerRuntime> runtime;
        {
            std::lock_guard<std::mutex> lock(detail::RuntimeMutex());
            runtime = std::move(detail::Runtime());
        }
        detail::StopPeriodicFlush(runtime);
        detail::Flush(runtime);
    }

    // Phase 3 callers used this spelling; retain it while they move to InitLogger.
    inline bool initLogger(const std::string& logger_name,
                        const std::string& logger_file,
                        spdlog::level::level_enum logger_level)
    {
        return InitLogger(logger_name, logger_file, logger_level);
    }
    #define OJ_LOG_AT(level, ...) \
    ::oj::logger::GetLogger()->log( \
        ::spdlog::source_loc{__FILE__, __LINE__, SPDLOG_FUNCTION}, level, __VA_ARGS__)
    #define LOG_DEBUG(...) OJ_LOG_AT(::spdlog::level::debug, __VA_ARGS__)
    #define LOG_INFO(...) OJ_LOG_AT(::spdlog::level::info, __VA_ARGS__)
    #define LOG_WARNING(...) OJ_LOG_AT(::spdlog::level::warn, __VA_ARGS__)
    #define LOG_ERROR(...) OJ_LOG_AT(::spdlog::level::err, __VA_ARGS__)
    #define LOG_CRITICAL(...) OJ_LOG_AT(::spdlog::level::critical, __VA_ARGS__)
}
