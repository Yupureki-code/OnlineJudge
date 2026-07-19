#pragma once

#include "../oj_cache.hpp"
#include "model_context.hpp"
#include <algorithm>
#include <atomic>
#include <cctype>
#include <string>


namespace oj::model
{
    using namespace oj::cache;
    class ModelBase
    {
    protected:
        bool IsAllDigits(const std::string& value) const
        {
            return !value.empty() && std::all_of(value.begin(), value.end(), [](unsigned char ch) {
                return std::isdigit(ch);
            });
        }

        bool IsValidQuestionId(const std::string& value) const
        {
            return !value.empty() && value.size() <= 5 &&
                std::all_of(value.begin(), value.end(), [](unsigned char ch) {
                    return std::isalnum(ch) != 0;
                });
        }

        std::string TrimCopy(const std::string& value) const
        {
            size_t begin = 0;
            size_t end = value.size();
            while (begin < end && std::isspace(static_cast<unsigned char>(value[begin]))) ++begin;
            while (end > begin && std::isspace(static_cast<unsigned char>(value[end - 1]))) --end;
            return value.substr(begin, end - begin);
        }

        std::string LowerAscii(const std::string& value) const
        {
            std::string result = value;
            std::transform(result.begin(), result.end(), result.begin(), [](unsigned char ch) {
                return static_cast<char>(std::tolower(ch));
            });
            return result;
        }

        ns_odb::ScopedDB AcquireDatabase()
        {
            EnsureDatabaseStarted();
            return ns_odb::ScopedDB(Context().Pool());
        }

        latecyMonitor::LatencyMonitor& Monitor() noexcept
        {
            return Context().Monitor();
        }

        static ModelContext& Context() noexcept
        {
            return ModelContext::Instance();
        }

        static bool EnsureDatabaseStarted()
        {
            if (Context().Started())
                return true;
            const std::string process = ns_runtime_cfg::GetEnvOrDefault("OJ_PROCESS_NAME", "oj_server");
            return Context().Start(process);
        }

        void InvalidateSolutionListCaches(const std::string& question_id)
        {
            _cache.BumpSolutionListVersion(question_id);
        }

    public:
        latecyMonitor::LatencyMonitor& GetLatencyMonitor() noexcept { return Monitor(); }

        static bool StartDatabase(const std::string& process_name)
        {
            return Context().Start(process_name);
        }

        static void StopDatabase() noexcept
        {
            Context().Stop();
        }

        enum class RecordActionType : int
        {
            Question = 0,
            Auth = 1,
            Comment = 2,
            Solution = 3,
            User = 4
        };

        struct CacheMetrics
        {
            struct ActionMetrics
            {
                std::atomic<long long> total_requests{0};
                std::atomic<long long> cache_hits{0};
                std::atomic<long long> db_fallbacks{0};
                std::atomic<long long> total_ms{0};
            };

            ActionMetrics actions[5];
            ActionMetrics& Get(RecordActionType type)
            {
                return actions[static_cast<int>(type)];
            }
        };

        static CacheMetrics& Metrics()
        {
            static CacheMetrics metrics;
            return metrics;
        }

        void RecordCacheMetrics(RecordActionType type, bool cache_hit,
                                bool db_fallback, long long cost_ms)
        {
            auto& metrics = Metrics().Get(type);
            metrics.total_requests.fetch_add(1, std::memory_order_relaxed);
            if (cache_hit) metrics.cache_hits.fetch_add(1, std::memory_order_relaxed);
            if (db_fallback) metrics.db_fallbacks.fetch_add(1, std::memory_order_relaxed);
            metrics.total_ms.fetch_add(cost_ms, std::memory_order_relaxed);
        }

        const std::string& SolutionsTable() const
        {
            static const std::string table = "solutions";
            return table;
        }

        Cache _cache;
    };
}
