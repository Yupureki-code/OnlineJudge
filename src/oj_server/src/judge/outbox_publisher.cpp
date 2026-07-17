#include "judge/outbox_publisher.hpp"

#include "model/model_submission.hpp"
#include "../../../comm/mq_client.hpp"

#include <algorithm>
#include <chrono>
#include <random>
#include <sstream>
#include <utility>

namespace oj::judge
{

bool ModelOutboxStore::Claim(std::size_t limit, const std::string& owner,
                             uint32_t lease_seconds, uint32_t confirm_timeout_seconds,
                             std::vector<oj::db::JudgeOutbox>* rows)
{
    return model_.ClaimPendingOutbox(limit, owner, lease_seconds, confirm_timeout_seconds, rows);
}

bool ModelOutboxStore::MarkPublished(const std::string& message_id, const std::string& owner)
{
    return model_.MarkOutboxPublished(message_id, owner);
}

bool ModelOutboxStore::MarkFailed(const std::string& message_id, const std::string& owner,
                                  uint32_t retry_delay_seconds)
{
    return model_.MarkOutboxFailed(message_id, owner, retry_delay_seconds);
}

bool ModelOutboxStore::CleanupExpiredReceipts()
{
    return model_.CleanupExpiredResultReceipts();
}

bool MqConfirmedPublisher::Publish(std::string message_id, std::string payload, Callback callback)
{
    return producer_.PublishConfirmed(std::move(message_id), std::move(payload), std::move(callback));
}

OutboxPublisher::OutboxPublisher(OutboxStore& store, ConfirmedMessagePublisher& publisher)
    : OutboxPublisher(store, publisher, Config{})
{
}

OutboxPublisher::OutboxPublisher(OutboxStore& store, ConfirmedMessagePublisher& publisher, Config config)
    : store_(store), publisher_(publisher), config_(config)
{
    if (config_.batch_size == 0) config_.batch_size = 1;
    std::random_device random;
    std::ostringstream owner;
    owner << std::hex << random() << random() << '-'
          << std::chrono::steady_clock::now().time_since_epoch().count();
    owner_ = owner.str();
}

OutboxPublisher::~OutboxPublisher()
{
    Stop();
}

bool OutboxPublisher::Start()
{
    bool expected = false;
    if (!running_.compare_exchange_strong(expected, true, std::memory_order_acq_rel)) return true;
    completions_->accepting.store(true, std::memory_order_release);
    try {
        worker_ = std::thread(&OutboxPublisher::Run, this);
        return true;
    } catch (...) {
        running_.store(false, std::memory_order_release);
        return false;
    }
}

void OutboxPublisher::Stop() noexcept
{
    running_.store(false, std::memory_order_release);
    wake_condition_.notify_all();
    completions_->condition.notify_all();
    if (worker_.joinable()) worker_.join();
    completions_->accepting.store(false, std::memory_order_release);
}

void OutboxPublisher::Notify()
{
    wake_condition_.notify_one();
}

void OutboxPublisher::Run()
{
    while (running_.load(std::memory_order_acquire)) {
        DrainCompletions();
        ExpireConfirms();
        const auto now = std::chrono::steady_clock::now();
        if (now >= next_cleanup_) {
            store_.CleanupExpiredReceipts();
            next_cleanup_ = now + std::chrono::hours(1);
        }
        std::vector<oj::db::JudgeOutbox> rows;
        if (store_.Claim(config_.batch_size, owner_, config_.lease_seconds,
                         config_.confirm_timeout_seconds, &rows)) {
            for (const auto& row : rows) {
                if (!running_.load(std::memory_order_acquire)) break;
                if (!in_flight_.emplace(row.message_id, InFlight{
                        row.attempt_count,
                        std::chrono::steady_clock::now() +
                            std::chrono::seconds(config_.confirm_timeout_seconds)}).second) continue;
                const std::string payload(row.payload.begin(), row.payload.end());
                const std::string message_id = row.message_id;
                const std::weak_ptr<SharedCompletions> weak = completions_;
                const bool accepted = publisher_.Publish(message_id, payload,
                    [weak, message_id](bool success, const std::string& error) {
                        const auto shared = weak.lock();
                        if (!shared || !shared->accepting.load(std::memory_order_acquire)) return;
                        {
                            std::lock_guard lock(shared->mutex);
                            shared->queue.push({message_id, success, error});
                        }
                        shared->condition.notify_one();
                    });
                if (!accepted) {
                    in_flight_.erase(message_id);
                    store_.MarkFailed(message_id, owner_, RetryDelay(row.attempt_count));
                }
            }
        }
        std::unique_lock lock(wake_mutex_);
        wake_condition_.wait_for(lock, config_.idle_wait, [this] {
            return !running_.load(std::memory_order_acquire);
        });
    }
    DrainCompletions();
}

void OutboxPublisher::DrainCompletions()
{
    std::queue<Completion> ready;
    {
        std::lock_guard lock(completions_->mutex);
        ready.swap(completions_->queue);
    }
    while (!ready.empty()) {
        Completion completion = std::move(ready.front());
        ready.pop();
        const auto in_flight = in_flight_.find(completion.message_id);
        if (in_flight == in_flight_.end()) continue;
        const uint32_t attempt_count = in_flight->second.attempt_count;
        const bool confirm_expired = in_flight->second.deadline <= std::chrono::steady_clock::now();
        in_flight_.erase(completion.message_id);
        if (completion.success && !confirm_expired)
            store_.MarkPublished(completion.message_id, owner_);
        else store_.MarkFailed(completion.message_id, owner_, RetryDelay(attempt_count));
    }
}

void OutboxPublisher::ExpireConfirms()
{
    const auto now = std::chrono::steady_clock::now();
    for (auto iterator = in_flight_.begin(); iterator != in_flight_.end();) {
        if (iterator->second.deadline > now) {
            ++iterator;
            continue;
        }
        const std::string message_id = iterator->first;
        const uint32_t attempt_count = iterator->second.attempt_count;
        iterator = in_flight_.erase(iterator);
        store_.MarkFailed(message_id, owner_, RetryDelay(attempt_count));
    }
}

uint32_t OutboxPublisher::RetryDelay(uint32_t attempt_count) const
{
    const uint32_t exponent = std::min(attempt_count, 10u);
    return std::min(config_.max_retry_delay_seconds, uint32_t{1} << exponent);
}

} // namespace oj::judge
