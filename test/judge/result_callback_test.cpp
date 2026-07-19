#include "comm/judge_callback_auth.hpp"

#include <cstdlib>
#include <iostream>
#include <string>

namespace
{
void Check(bool condition, const char* message)
{
    if (condition) return;
    std::cerr << "FAIL: " << message << '\n';
    std::exit(1);
}
}

int main()
{
    const std::string secret = "test-callback-secret";
    oj::biz::UpdateJudgeResultRequest request;
    request.set_message_id("message-1");
    request.set_submission_id(99);
    request.set_result_payload("result\0payload", 14);
    request.mutable_service_auth()->set_key_id("key-1");
    request.mutable_service_auth()->set_timestamp(1700000000);
    request.mutable_service_auth()->set_nonce("0123456789abcdef0123456789abcdef");
    request.mutable_service_auth()->set_hmac_sha256(
        oj::security::JudgeCallbackHmac(request, secret));
    Check(oj::security::VerifyJudgeCallbackHmac(request, secret), "valid raw-payload HMAC verifies");

    auto changed = request;
    changed.set_message_id("message-2");
    Check(!oj::security::VerifyJudgeCallbackHmac(changed, secret), "message id is signed");
    changed = request;
    changed.set_custom_task_id("custom-1");
    Check(!oj::security::VerifyJudgeCallbackHmac(changed, secret), "typed target is signed");
    changed = request;
    changed.mutable_service_auth()->set_timestamp(1700000001);
    Check(!oj::security::VerifyJudgeCallbackHmac(changed, secret), "timestamp is signed");
    changed = request;
    changed.mutable_service_auth()->set_nonce("fedcba9876543210fedcba9876543210");
    Check(!oj::security::VerifyJudgeCallbackHmac(changed, secret), "nonce is signed");
    changed = request;
    changed.set_result_payload("different");
    Check(!oj::security::VerifyJudgeCallbackHmac(changed, secret), "result bytes are signed");
    Check(!oj::security::VerifyJudgeCallbackHmac(request, "wrong-secret"), "wrong key is rejected");
    std::cout << "result_callback_test: PASS\n";
}
