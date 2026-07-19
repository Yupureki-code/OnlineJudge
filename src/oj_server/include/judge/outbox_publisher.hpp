#pragma once

#include "../../../comm/models/judge_outbox.hxx"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

namespace ns_mq { class MQProducer; }
namespace oj::model { class ModelSubmission; }

namespace oj::judge
{

class OutboxStore
{
public:
    virtual ~OutboxStore() = default;
    virtual bool Claim(std::size_t limit, const std::string& owner,
                       uint32_t lease_seconds, uint32_t confirm_timeout_seconds,
                       std::vector<oj::db::JudgeOutbox>* rows) = 0;
    virtual bool MarkPublished(const std::string& message_id, const std::string& owner) = 0;
    virtual bool MarkFailed(const std::string& message_id, const std::string& owner,
                            uint32_t retry_delay_seconds) = 0;
    virtual bool CleanupExpiredReceipts() { return true; }
};

class ConfirmedMessagePublisher
{
public:
    using Callback = std::function<void(bool, const std::string&)>;
    virtual ~ConfirmedMessagePublisher() = default;
    virtual bool Publish(std::string message_id, std::string payload, Callback callback) = 0;
};

class ModelOutboxStore final : public OutboxStore
{
public:
    explicit ModelOutboxStore(oj::model::ModelSubmission& model) : model_(model) {}
    bool Claim(std::size_t limit, const std::string& owner,
               uint32_t lease_seconds, uint32_t confirm_timeout_seconds,
               std::vector<oj::db::JudgeOutbox>* rows) override;
    bool MarkPublished(const std::string& message_id, const std::string& owner) override;
    bool MarkFailed(const std::string& message_id, const std::string& owner,
                    uint32_t retry_delay_seconds) override;
    bool CleanupExpiredReceipts() override;

private:
    oj::model::ModelSubmission& model_;
};

class MqConfirmedPublisher final : public ConfirmedMessagePublisher
{
public:
    explicit MqConfirmedPublisher(ns_mq::MQProducer& producer) : producer_(producer) {}
    bool Publish(std::string message_id, std::string payload, Callback callback) override;

private:
    ns_mq::MQProducer& producer_;
};

class OutboxPublisher
{
public:
    struct Config
    {
        std::size_t batch_size = 32;
        std::chrono::milliseconds idle_wait{1000};
        uint32_t max_retry_delay_seconds = 60;
        uint32_t lease_seconds = 30;
        uint32_t confirm_timeout_seconds = 5;
    };

    OutboxPublisher(OutboxStore& store, ConfirmedMessagePublisher& publisher);
    OutboxPublisher(OutboxStore& store, ConfirmedMessagePublisher& publisher, Config config);
    ~OutboxPublisher();
    bool Start();
    void Stop() noexcept;
    void Notify();

private:
    struct Completion { std::string message_id; bool success; std::string error; };
    struct InFlight
    {
        uint32_t attempt_count = 0;
        std::chrono::steady_clock::time_point deadline;
    };
    struct SharedCompletions
    {
        std::mutex mutex;
        std::condition_variable condition;
        std::queue<Completion> queue;
        bool wake_requested = false;
        std::atomic<bool> accepting{true};
    };

    void Run();
    void DrainCompletions();
    void ExpireConfirms();
    uint32_t RetryDelay(uint32_t attempt_count) const;

    OutboxStore& store_;
    ConfirmedMessagePublisher& publisher_;
    Config config_;
    std::shared_ptr<SharedCompletions> completions_ = std::make_shared<SharedCompletions>();
    std::unordered_map<std::string, InFlight> in_flight_;
    std::string owner_;
    std::chrono::steady_clock::time_point next_cleanup_{};
    std::atomic<bool> running_{false};
    std::thread worker_;
};

} // namespace oj::judge
