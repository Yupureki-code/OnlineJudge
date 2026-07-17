#include "judge/outbox_publisher.hpp"

#include <atomic>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <thread>

namespace
{

using namespace std::chrono_literals;

void Check(bool condition, const char* message)
{
    if (condition) return;
    std::cerr << "FAIL: " << message << '\n';
    std::exit(1);
}

class FakeStore final : public oj::judge::OutboxStore
{
public:
    bool Claim(std::size_t, const std::string& owner, uint32_t, uint32_t,
               std::vector<oj::db::JudgeOutbox>* rows) override
    {
        if (!finished.load(std::memory_order_acquire)) {
            oj::db::JudgeOutbox row;
            row.message_id = "message-1";
            row.payload = {'t', 'a', 's', 'k'};
            row.attempt_count = 0;
            claimed_owner = owner;
            rows->push_back(std::move(row));
        }
        return true;
    }

    bool MarkPublished(const std::string& message_id, const std::string& owner) override
    {
        published = message_id == "message-1" && owner == claimed_owner;
        finished.store(true, std::memory_order_release);
        return true;
    }

    bool MarkFailed(const std::string& message_id, const std::string& owner,
                    uint32_t delay) override
    {
        failed = message_id == "message-1" && owner == claimed_owner && delay > 0;
        finished.store(true, std::memory_order_release);
        return true;
    }

    std::atomic<bool> finished{false};
    bool published = false;
    bool failed = false;
    std::string claimed_owner;
};

class FakePublisher final : public oj::judge::ConfirmedMessagePublisher
{
public:
    explicit FakePublisher(bool success, bool accepts = true) : success_(success), accepts_(accepts) {}

    bool Publish(std::string message_id, std::string payload, Callback callback) override
    {
        ++calls;
        Check(message_id == "message-1", "publisher receives message id");
        Check(payload == "task", "publisher receives immutable payload");
        if (!accepts_) return false;
        callback(success_, success_ ? "" : "NACK");
        return true;
    }

    std::atomic<int> calls{0};

private:
    bool success_;
    bool accepts_;
};

class LostConfirmPublisher final : public oj::judge::ConfirmedMessagePublisher
{
public:
    bool Publish(std::string, std::string, Callback) override
    {
        ++calls;
        return true;
    }
    std::atomic<int> calls{0};
};

void RunCase(bool success, bool accepts)
{
    FakeStore store;
    FakePublisher publisher(success, accepts);
    oj::judge::OutboxPublisher::Config config;
    config.idle_wait = 5ms;
    config.batch_size = 1;
    config.confirm_timeout_seconds = 1;
    oj::judge::OutboxPublisher worker(store, publisher, config);
    Check(worker.Start(), "publisher worker starts");
    worker.Notify();
    for (int attempt = 0; attempt < 100 && !store.finished.load(); ++attempt)
        std::this_thread::sleep_for(5ms);
    worker.Stop();
    Check(store.finished.load(), "outbox reaches a terminal publish attempt");
    Check(publisher.calls.load() == 1, "in-flight row is published once");
    Check(store.published == (success && accepts), "confirmed publish is marked published");
    Check(store.failed == (!success || !accepts), "failed publish is rescheduled");
}

void RunRestartRecoveryCase()
{
    FakeStore store;
    LostConfirmPublisher lost_confirm;
    oj::judge::OutboxPublisher::Config config;
    config.idle_wait = 5ms;
    config.batch_size = 1;
    config.confirm_timeout_seconds = 1;
    {
        oj::judge::OutboxPublisher worker(store, lost_confirm, config);
        Check(worker.Start(), "first publisher starts");
        std::this_thread::sleep_for(30ms);
        worker.Stop();
    }
    Check(!store.finished.load(), "missing confirm remains claimed before deadline");
    Check(lost_confirm.calls.load() == 1, "in-flight event is not duplicated before restart");

    FakePublisher recovered(true);
    {
        oj::judge::OutboxPublisher worker(store, recovered, config);
        Check(worker.Start(), "restarted publisher starts");
        for (int attempt = 0; attempt < 100 && !store.finished.load(); ++attempt)
            std::this_thread::sleep_for(5ms);
        worker.Stop();
    }
    Check(store.published, "restart republishes the stable pending event");
    Check(recovered.calls.load() == 1, "restart performs one confirmed republish");
}

} // namespace

int main()
{
    RunCase(true, true);
    RunCase(false, true);
    RunCase(false, false);
    RunRestartRecoveryCase();
    std::cout << "outbox_publisher_test: PASS\n";
    return 0;
}
