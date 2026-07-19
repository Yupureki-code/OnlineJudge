#pragma once

#include "filesystem.hpp"
#include "logger.hpp"
#include "comm.hpp"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <limits>
#include <mutex>
#include <queue>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

namespace latecyMonitor
{
    struct LatencyMonitorConfig
    {
        std::string output_path = "./log/latency";
        std::string file_name = "process_latency.csv";
        size_t batch_size = 1000;
        std::chrono::milliseconds flush_interval{5000};
        size_t sample_per_mille = 1000;
        size_t max_queue_records = 10000;
        size_t max_file_bytes = 64 * 1024 * 1024;
        size_t max_files = 5;
    };

    inline size_t PositiveEnvironmentSize(const char* name, size_t fallback)
    {
        const char* value = std::getenv(name);
        if (value == nullptr || *value == '\0')
            return fallback;

        for (const char* current = value; *current != '\0'; ++current) {
            if (*current < '0' || *current > '9')
                return fallback;
        }

        try {
            size_t parsed_characters = 0;
            const unsigned long long parsed = std::stoull(value, &parsed_characters);
            if (parsed_characters != std::string(value).size() || parsed == 0 ||
                parsed > std::numeric_limits<size_t>::max())
                return fallback;
            return static_cast<size_t>(parsed);
        } catch (const std::exception&) {
            return fallback;
        }
    }

    inline LatencyMonitorConfig LatencyConfigFromEnv(const std::string& process_name)
    {
        LatencyMonitorConfig config;
        if (const char* path = std::getenv("OJ_LATENCY_PATH"); path != nullptr && *path != '\0')
            config.output_path = path;
        if (!process_name.empty())
            config.file_name = process_name + "_latency.csv";
        if (process_name == "oj_server" || process_name == "oj_admin")
            config.sample_per_mille = 100;
        config.batch_size = PositiveEnvironmentSize("OJ_LATENCY_BATCH_SIZE", config.batch_size);
        config.flush_interval = std::chrono::milliseconds(
            PositiveEnvironmentSize("OJ_LATENCY_FLUSH_MS",
                                    static_cast<size_t>(config.flush_interval.count())));
        config.sample_per_mille = std::min<size_t>(1000,
            PositiveEnvironmentSize("OJ_LATENCY_SAMPLE_PERMILLE", config.sample_per_mille));
        config.max_queue_records = PositiveEnvironmentSize(
            "OJ_LATENCY_MAX_QUEUE_RECORDS", config.max_queue_records);
        config.max_file_bytes = PositiveEnvironmentSize(
            "OJ_LATENCY_MAX_FILE_BYTES", config.max_file_bytes);
        config.max_files = PositiveEnvironmentSize("OJ_LATENCY_MAX_FILES", config.max_files);
        return config;
    }

    class LatencyMonitor
    {
    private:
        struct LatencyRecord
        {
            std::string operation;
            int64_t latency_ms;
            int64_t timestamp_us;

            LatencyRecord(std::string op, int64_t lat, int64_t ts)
                : operation(std::move(op)), latency_ms(lat), timestamp_us(ts) {}
        };

        void worker_thread_func()
        {
            for (;;) {
                std::vector<LatencyRecord> batch;
                {
                    std::unique_lock<std::mutex> lock(queue_mutex_);
                    cv_.wait_for(lock, flush_interval_, [this] {
                        return !running_.load() || queue_.size() >= batch_size_;
                    });

                    const size_t count = std::min(batch_size_, queue_.size());
                    batch.reserve(count);
                    for (size_t i = 0; i < count; ++i) {
                        batch.push_back(std::move(queue_.front()));
                        queue_.pop();
                    }

                    if (batch.empty() && !running_.load())
                        break;
                }

                if (!batch.empty())
                    write_batch(batch);
            }
        }

        bool write_batch(const std::vector<LatencyRecord>& batch)
        {
            std::stringstream contents;
            for (const auto& record : batch)
                contents << oj::util::TimeUtil::GetTimeString(record.timestamp_us) << ','
                         << record.operation << ',' << record.latency_ms << '\n';

            fileUtil::Response response;
            {
                std::lock_guard<std::mutex> lock(file_mutex_);
                std::string full_path = _file_path.empty() ? "." : _file_path;
                if (!full_path.empty() && full_path.back() != '/')
                    full_path.push_back('/');
                const std::string output = contents.str();
                const std::filesystem::path output_path = full_path + _file_name;
                std::error_code error;
                const auto current_size = std::filesystem::exists(output_path, error)
                    ? std::filesystem::file_size(output_path, error) : 0;
                const size_t max_bytes = max_file_bytes_.load(std::memory_order_relaxed);
                const size_t max_files = max_files_.load(std::memory_order_relaxed);
                if (!error && current_size + output.size() > max_bytes) {
                    if (max_files <= 1) {
                        std::filesystem::remove(output_path, error);
                    } else {
                        const auto last = output_path.string() + "." + std::to_string(max_files - 1);
                        std::filesystem::remove(last, error);
                        for (size_t index = max_files - 1; index > 1; --index) {
                            const auto source = output_path.string() + "." + std::to_string(index - 1);
                            const auto target = output_path.string() + "." + std::to_string(index);
                            error.clear();
                            if (std::filesystem::exists(source, error))
                                std::filesystem::rename(source, target, error);
                        }
                        error.clear();
                        std::filesystem::rename(output_path, output_path.string() + ".1", error);
                    }
                }
                response = _file.append(output_path.string(), output);
            }
            if (!response.status) {
                LOG_ERROR("LatencyMonitor: {}", response.errmsg);
                return false;
            }
            return true;
        }

        static int64_t current_timestamp_us()
        {
            return std::chrono::duration_cast<std::chrono::microseconds>(
                       std::chrono::system_clock::now().time_since_epoch())
                .count();
        }

    public:
        LatencyMonitor() = default;
        ~LatencyMonitor() { stop(); }

        LatencyMonitor(const LatencyMonitor&) = delete;
        LatencyMonitor& operator=(const LatencyMonitor&) = delete;

        bool start()
        {
            std::lock_guard<std::mutex> lifecycle_lock(lifecycle_mutex_);
            if (running_.load())
                return false;

            {
                std::lock_guard<std::mutex> queue_lock(queue_mutex_);
                running_.store(true);
            }
            try {
                worker_thread_ = std::thread(&LatencyMonitor::worker_thread_func, this);
            } catch (...) {
                running_.store(false);
                throw;
            }
            return true;
        }

        void stop()
        {
            std::lock_guard<std::mutex> lifecycle_lock(lifecycle_mutex_);
            {
                std::lock_guard<std::mutex> queue_lock(queue_mutex_);
                running_.store(false);
                enabled_.store(false);
            }
            cv_.notify_all();
            if (worker_thread_.joinable())
                worker_thread_.join();
        }

        void enable(bool on) { enabled_.store(on); }

        bool setOutputFile(const std::string& file_path, const std::string& file_name)
        {
            if (file_name.empty())
                return false;
            std::lock_guard<std::mutex> lock(file_mutex_);
            _file_path = file_path;
            _file_name = file_name;
            return true;
        }

        bool configure(const LatencyMonitorConfig& config)
        {
            if (!setOutputFile(config.output_path, config.file_name))
                return false;
            set_batch_size(config.batch_size);
            set_flush_interval(config.flush_interval);
            sample_per_mille_.store(config.sample_per_mille, std::memory_order_relaxed);
            max_queue_records_.store(config.max_queue_records, std::memory_order_relaxed);
            max_file_bytes_.store(config.max_file_bytes, std::memory_order_relaxed);
            max_files_.store(config.max_files, std::memory_order_relaxed);
            return true;
        }

        bool configureFromEnv(const std::string& process_name)
        {
            return configure(LatencyConfigFromEnv(process_name));
        }

        void record(const std::string& operation, int64_t latency_ms)
        {
            {
                std::lock_guard<std::mutex> lock(queue_mutex_);
                if (!enabled_.load() || !running_.load())
                    return;
                if (queue_.size() >= max_queue_records_.load(std::memory_order_relaxed)) {
                    dropped_records_.fetch_add(1, std::memory_order_relaxed);
                    return;
                }
                queue_.emplace(operation, latency_ms, current_timestamp_us());
                accepted_records_.fetch_add(1, std::memory_order_relaxed);
            }
            cv_.notify_one();
        }

        bool is_enabled() const { return enabled_.load(); }
        bool is_running() const { return running_.load(); }
        bool should_sample()
        {
            const size_t rate = sample_per_mille_.load(std::memory_order_relaxed);
            return rate >= 1000 || (rate > 0 &&
                sample_sequence_.fetch_add(1, std::memory_order_relaxed) % 1000 < rate);
        }
        uint64_t accepted_records() const { return accepted_records_.load(std::memory_order_relaxed); }
        uint64_t dropped_records() const { return dropped_records_.load(std::memory_order_relaxed); }

        size_t queue_size() const
        {
            std::lock_guard<std::mutex> lock(queue_mutex_);
            return queue_.size();
        }

        void set_batch_size(size_t size)
        {
            {
                std::lock_guard<std::mutex> lock(queue_mutex_);
                batch_size_ = std::max<size_t>(1, size);
            }
            cv_.notify_all();
        }

        void set_flush_interval(std::chrono::milliseconds interval)
        {
            {
                std::lock_guard<std::mutex> lock(queue_mutex_);
                flush_interval_ = std::max(std::chrono::milliseconds(1), interval);
            }
            cv_.notify_all();
        }

    private:
        std::atomic<bool> enabled_{false};
        std::atomic<bool> running_{false};
        std::queue<LatencyRecord> queue_;
        mutable std::mutex queue_mutex_;
        std::condition_variable cv_;
        std::mutex lifecycle_mutex_;
        std::thread worker_thread_;
        std::string _file_path = "./log/latency";
        std::string _file_name = "process_latency.csv";
        fileUtil::FileSystem _file;
        mutable std::mutex file_mutex_;
        size_t batch_size_ = 1000;
        std::chrono::milliseconds flush_interval_{5000};
        std::atomic<size_t> sample_per_mille_{1000};
        std::atomic<size_t> max_queue_records_{10000};
        std::atomic<size_t> max_file_bytes_{64 * 1024 * 1024};
        std::atomic<size_t> max_files_{5};
        std::atomic<uint64_t> sample_sequence_{0};
        std::atomic<uint64_t> accepted_records_{0};
        std::atomic<uint64_t> dropped_records_{0};
    };

    class Timer
    {
    public:
        Timer(LatencyMonitor& monitor, const std::string& operation)
            : monitor_(monitor), operation_(operation),
              enabled_(monitor.is_enabled() && monitor.should_sample())
        {
            if (enabled_)
                start_ = std::chrono::steady_clock::now();
        }

        ~Timer()
        {
            if (enabled_) {
                const auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                                          std::chrono::steady_clock::now() - start_)
                                          .count();
                monitor_.record(operation_, duration);
            }
        }

        Timer(const Timer&) = delete;
        Timer& operator=(const Timer&) = delete;

    private:
        LatencyMonitor& monitor_;
        std::string operation_;
        std::chrono::steady_clock::time_point start_;
        bool enabled_;
    };
}
