#include "judge_inbox.hpp"

#include <atomic>
#include <barrier>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

namespace
{

void Check(bool condition, const char* message)
{
    if (condition) return;
    std::cerr << "FAIL: " << message << '\n';
    std::exit(1);
}

} // namespace

int main(int argc, char** argv)
{
    const char* configured_uri = std::getenv("OJ_REDIS_ADDR");
    const std::string uri = argc > 1 ? argv[1] :
        (configured_uri && *configured_uri ? configured_uri : "tcp://127.0.0.1:6379");
    const std::string prefix = "oj:test:judge-inbox:" + std::to_string(
        std::chrono::steady_clock::now().time_since_epoch().count()) + ":";

    try {
        oj::judge::JudgeInbox first;
        oj::judge::JudgeInbox second;
        first.Open(uri, prefix, 80, 1000);
        second.Open(uri, prefix, 80, 1000);

        const std::string payload("task\0payload", 12);
        const std::string fingerprint = oj::judge::JudgeInbox::PayloadFingerprint(payload);
        oj::common::JudgeResult result;
        std::string owner;
        Check(first.Claim("message-1", fingerprint, 60000, &result, &owner) ==
              oj::judge::JudgeInbox::ClaimResult::Started, "new delivery is claimed");
        std::string duplicate_owner;
        uint32_t retry_after_ms = 0;
        Check(second.Claim("message-1", fingerprint, 60000, &result, &duplicate_owner,
                           &retry_after_ms) == oj::judge::JudgeInbox::ClaimResult::Processing,
              "another instance cannot claim an active lease");
        Check(retry_after_ms >= 500 && retry_after_ms <= 5000,
              "processing claim supplies bounded delayed retry");
        Check(second.Claim("message-1", oj::judge::JudgeInbox::PayloadFingerprint("different"),
                           60000, &result, &duplicate_owner) ==
              oj::judge::JudgeInbox::ClaimResult::Conflict,
              "message id is bound to its payload fingerprint");

        result.set_status(oj::common::SUBMISSION_STATUS_ACCEPTED);
        result.set_compile_error(std::string("binary\0result", 13));
        result.set_completed_at(1234);
        Check(first.Complete("message-1", fingerprint, owner, result) ==
              oj::judge::JudgeInbox::CompleteResult::Completed, "owner persists completion");
        oj::common::JudgeResult recovered;
        Check(second.Claim("message-1", fingerprint, 60000, &recovered, &duplicate_owner) ==
              oj::judge::JudgeInbox::ClaimResult::Completed,
              "completion is shared across instances");
        Check(recovered.SerializeAsString() == result.SerializeAsString(),
              "binary protobuf result round-trips unchanged");

        oj::common::JudgeResult stale_result;
        std::string stale_owner;
        Check(first.Claim("message-2", fingerprint, 20, &stale_result, &stale_owner) ==
              oj::judge::JudgeInbox::ClaimResult::Started, "short lease is claimed");
        std::this_thread::sleep_for(std::chrono::milliseconds(30));
        std::string replacement_owner;
        Check(second.Claim("message-2", fingerprint, 60000, &stale_result,
                           &replacement_owner) == oj::judge::JudgeInbox::ClaimResult::Started,
              "expired lease is atomically reclaimed");
        Check(first.Complete("message-2", fingerprint, stale_owner, result) ==
              oj::judge::JudgeInbox::CompleteResult::NotOwner,
              "stale owner is fenced from completion");
        Check(second.Complete("message-2", fingerprint, replacement_owner, result) ==
              oj::judge::JudgeInbox::CompleteResult::Completed,
              "replacement owner completes");

        std::barrier start_line(3);
        std::atomic<int> started{0};
        std::atomic<int> processing{0};
        auto compete = [&](oj::judge::JudgeInbox& inbox) {
            oj::common::JudgeResult ignored;
            std::string competing_owner;
            start_line.arrive_and_wait();
            const auto claim = inbox.Claim("message-3", fingerprint, 60000, &ignored,
                                           &competing_owner);
            if (claim == oj::judge::JudgeInbox::ClaimResult::Started) ++started;
            if (claim == oj::judge::JudgeInbox::ClaimResult::Processing) ++processing;
        };
        std::thread contender_one(compete, std::ref(first));
        std::thread contender_two(compete, std::ref(second));
        start_line.arrive_and_wait();
        contender_one.join();
        contender_two.join();
        Check(started == 1 && processing == 1,
              "concurrent instances produce exactly one execution owner");

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        std::string after_ttl_owner;
        Check(first.Claim("message-1", fingerprint, 60000, &recovered, &after_ttl_owner) ==
              oj::judge::JudgeInbox::ClaimResult::Started,
              "completed records are removed by TTL");
    } catch (const std::exception& error) {
        std::cerr << "judge_inbox_test requires reachable Redis URI (argument or OJ_REDIS_ADDR): "
                  << error.what() << '\n';
        return 2;
    }

    std::cout << "judge_inbox_test: PASS\n";
    return 0;
}
