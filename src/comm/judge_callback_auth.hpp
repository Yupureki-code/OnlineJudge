#pragma once

#include "proto/biz_service.pb.h"

#include <openssl/crypto.h>
#include <openssl/hmac.h>

#include <string>

namespace oj::security
{

inline std::string JudgeCallbackTarget(const oj::biz::UpdateJudgeResultRequest& request)
{
    if (request.has_submission_id()) return "submission:" + std::to_string(request.submission_id());
    if (request.has_custom_task_id()) return "custom:" + request.custom_task_id();
    return {};
}

inline std::string JudgeCallbackSigningInput(const oj::biz::UpdateJudgeResultRequest& request)
{
    return "oj-judge-result-v1\n" + request.message_id() + "\n" + JudgeCallbackTarget(request) + "\n" +
           std::to_string(request.service_auth().timestamp()) + "\n" +
           request.service_auth().nonce() + "\n" + request.result_payload();
}

inline std::string JudgeCallbackHmac(const oj::biz::UpdateJudgeResultRequest& request,
                                     const std::string& secret)
{
    const std::string input = JudgeCallbackSigningInput(request);
    unsigned int length = 0;
    unsigned char digest[EVP_MAX_MD_SIZE];
    HMAC(EVP_sha256(), secret.data(), static_cast<int>(secret.size()),
         reinterpret_cast<const unsigned char*>(input.data()), input.size(), digest, &length);
    return {reinterpret_cast<const char*>(digest), length};
}

inline bool VerifyJudgeCallbackHmac(const oj::biz::UpdateJudgeResultRequest& request,
                                    const std::string& secret)
{
    const std::string expected = JudgeCallbackHmac(request, secret);
    const std::string& actual = request.service_auth().hmac_sha256();
    return expected.size() == actual.size() && !expected.empty() &&
           CRYPTO_memcmp(expected.data(), actual.data(), expected.size()) == 0;
}

} // namespace oj::security
