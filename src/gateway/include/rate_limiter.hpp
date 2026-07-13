#pragma once
#include <string>
#include <chrono>
#include <mutex>
#include <unordered_map>

namespace ns_gateway {

/// 令牌桶限流器
/// 基于 Redis 的生产环境限流 + 内存清理机制
class RateLimiter {
public:
    RateLimiter(int max_requests, int window_s);
    bool Allow(const std::string& key);
    void CleanupExpired();

private:
    struct Bucket { int count = 0; int64_t window_start = 0; };
    int _max_requests;
    int _window_s;
    int64_t _last_cleanup = 0;
    std::mutex _mtx;
    std::unordered_map<std::string, Bucket> _buckets;
};

} // namespace ns_gateway