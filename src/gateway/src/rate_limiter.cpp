#include "../include/rate_limiter.hpp"
#include <algorithm>

namespace ns_gateway {

RateLimiter::RateLimiter(int max_requests, int window_s)
    : _max_requests(max_requests), _window_s(window_s) {
    _last_cleanup = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
}

bool RateLimiter::Allow(const std::string& key) {
    std::lock_guard<std::mutex> lock(_mtx);
    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();

    // 定期清理过期桶（每 5 分钟一次）
    if (now - _last_cleanup > 300000) {
        CleanupExpired();
        _last_cleanup = now;
    }

    auto& bucket = _buckets[key];
    int64_t elapsed_s = (now - bucket.window_start) / 1000;

    if (elapsed_s >= _window_s) {
        bucket.count = 0;
        bucket.window_start = now;
    }

    if (bucket.count >= _max_requests) return false;
    bucket.count++;
    return true;
}

void RateLimiter::CleanupExpired() {
    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
    for (auto it = _buckets.begin(); it != _buckets.end(); ) {
        if ((now - it->second.window_start) / 1000 > _window_s * 2) {
            it = _buckets.erase(it);
        } else {
            ++it;
        }
    }
}

} // namespace ns_gateway