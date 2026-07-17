#pragma once

#include <openssl/sha.h>
#include <openssl/rand.h>
#include <sw/redis++/redis++.h>

#include <algorithm>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "../../comm/proto/common.pb.h"

namespace oj::judge
{

class JudgeInbox
{
public:
    enum class ClaimResult { Started, Processing, Completed, Conflict, Error };
    enum class CompleteResult { Completed, NotOwner, Conflict, Error };

    void Open(const std::string& redis_uri, std::string key_prefix = "oj:prod:v1:judge:inbox:",
              uint64_t completed_ttl_ms = 7ULL * 24 * 60 * 60 * 1000,
              uint64_t processing_retention_ms = 60000)
    {
        if (redis_uri.empty() || key_prefix.empty() || completed_ttl_ms == 0)
            throw std::invalid_argument("invalid Redis judge inbox configuration");
        auto redis = std::make_unique<sw::redis::Redis>(redis_uri);
        if (redis->ping() != "PONG") throw std::runtime_error("Redis judge inbox ping failed");
        redis_ = std::move(redis);
        key_prefix_ = std::move(key_prefix);
        completed_ttl_ms_ = completed_ttl_ms;
        processing_retention_ms_ = processing_retention_ms;
    }

    ClaimResult Claim(const std::string& message_id, const std::string& fingerprint,
                      uint32_t lease_ms, oj::common::JudgeResult* completed,
                      std::string* claim_token, uint32_t* retry_after_ms = nullptr)
    {
        if (message_id.empty() || fingerprint.empty() || lease_ms == 0 || completed == nullptr ||
            claim_token == nullptr || !redis_) return ClaimResult::Error;
        claim_token->clear();
        if (retry_after_ms) *retry_after_ms = 1000;
        const std::string owner = RandomToken();
        if (owner.empty()) return ClaimResult::Error;
        const std::vector<std::string> keys{Key(message_id)};
        const std::vector<std::string> args{
            fingerprint, owner, std::to_string(lease_ms),
            std::to_string(static_cast<uint64_t>(lease_ms) + processing_retention_ms_)};
        try {
            const auto reply = redis_->eval<std::vector<std::string>>(
                ClaimScript(), keys.begin(), keys.end(), args.begin(), args.end());
            if (reply.empty()) return ClaimResult::Error;
            if (reply[0] == "STARTED") {
                *claim_token = owner;
                return ClaimResult::Started;
            }
            if (reply[0] == "PROCESSING") {
                if (retry_after_ms && reply.size() > 1) {
                    const auto remaining = static_cast<uint64_t>(std::stoull(reply[1]));
                    *retry_after_ms = static_cast<uint32_t>(std::clamp<uint64_t>(remaining, 500, 5000));
                }
                return ClaimResult::Processing;
            }
            if (reply[0] == "COMPLETED" && reply.size() > 1) {
                return completed->ParseFromString(reply[1]) ? ClaimResult::Completed
                                                            : ClaimResult::Error;
            }
            if (reply[0] == "CONFLICT") return ClaimResult::Conflict;
        } catch (const std::exception&) {
            return ClaimResult::Error;
        }
        return ClaimResult::Error;
    }

    CompleteResult Complete(const std::string& message_id, const std::string& fingerprint,
                            const std::string& claim_token,
                            const oj::common::JudgeResult& result)
    {
        std::string payload;
        if (message_id.empty() || fingerprint.empty() || claim_token.empty() || !redis_ ||
            !result.SerializeToString(&payload)) return CompleteResult::Error;
        const std::vector<std::string> keys{Key(message_id)};
        const std::vector<std::string> args{fingerprint, claim_token, payload,
                                            std::to_string(completed_ttl_ms_)};
        try {
            const auto reply = redis_->eval<std::string>(
                CompleteScript(), keys.begin(), keys.end(), args.begin(), args.end());
            if (reply == "COMPLETED") return CompleteResult::Completed;
            if (reply == "CONFLICT") return CompleteResult::Conflict;
            if (reply == "NOT_OWNER") return CompleteResult::NotOwner;
        } catch (const std::exception&) {
            return CompleteResult::Error;
        }
        return CompleteResult::Error;
    }

    static std::string PayloadFingerprint(const std::string& payload)
    {
        unsigned char digest[SHA256_DIGEST_LENGTH];
        SHA256(reinterpret_cast<const unsigned char*>(payload.data()), payload.size(), digest);
        static constexpr char hex[] = "0123456789abcdef";
        std::string output(SHA256_DIGEST_LENGTH * 2, '0');
        for (std::size_t index = 0; index < SHA256_DIGEST_LENGTH; ++index) {
            output[index * 2] = hex[digest[index] >> 4];
            output[index * 2 + 1] = hex[digest[index] & 0x0f];
        }
        return output;
    }

private:
    static const std::string& ClaimScript()
    {
        static const std::string script = R"lua(
local key = KEYS[1]
local fingerprint = ARGV[1]
local owner = ARGV[2]
local lease_ms = tonumber(ARGV[3])
local retention_ms = tonumber(ARGV[4])
local now_parts = redis.call('TIME')
local now = now_parts[1] * 1000 + math.floor(now_parts[2] / 1000)
local state = redis.call('HGET', key, 'state')
if state then
    if redis.call('HGET', key, 'fingerprint') ~= fingerprint then
        return {'CONFLICT'}
    end
    if state == 'completed' then
        return {'COMPLETED', redis.call('HGET', key, 'result')}
    end
    if state ~= 'processing' then
        return {'ERROR'}
    end
    local lease_until = tonumber(redis.call('HGET', key, 'lease_until') or '0')
    if lease_until > now then
        return {'PROCESSING', tostring(lease_until - now)}
    end
end
redis.call('HSET', key, 'state', 'processing', 'fingerprint', fingerprint,
           'owner', owner, 'lease_until', tostring(now + lease_ms))
redis.call('HDEL', key, 'result')
redis.call('PEXPIRE', key, retention_ms)
return {'STARTED'}
)lua";
        return script;
    }

    static const std::string& CompleteScript()
    {
        static const std::string script = R"lua(
local key = KEYS[1]
local fingerprint = ARGV[1]
local owner = ARGV[2]
local result = ARGV[3]
local completed_ttl_ms = tonumber(ARGV[4])
local state = redis.call('HGET', key, 'state')
if not state then return 'NOT_OWNER' end
if redis.call('HGET', key, 'fingerprint') ~= fingerprint then return 'CONFLICT' end
if state == 'completed' then
    if redis.call('HGET', key, 'result') == result then return 'COMPLETED' end
    return 'NOT_OWNER'
end
local now_parts = redis.call('TIME')
local now = now_parts[1] * 1000 + math.floor(now_parts[2] / 1000)
local lease_until = tonumber(redis.call('HGET', key, 'lease_until') or '0')
if state ~= 'processing' or redis.call('HGET', key, 'owner') ~= owner or lease_until <= now then
    return 'NOT_OWNER'
end
redis.call('HSET', key, 'state', 'completed', 'result', result)
redis.call('HDEL', key, 'owner', 'lease_until')
redis.call('PEXPIRE', key, completed_ttl_ms)
return 'COMPLETED'
)lua";
        return script;
    }

    static std::string RandomToken()
    {
        unsigned char random[16];
        if (RAND_bytes(random, sizeof(random)) != 1) return {};
        static constexpr char hex[] = "0123456789abcdef";
        std::string token(sizeof(random) * 2, '0');
        for (std::size_t index = 0; index < sizeof(random); ++index) {
            token[index * 2] = hex[random[index] >> 4];
            token[index * 2 + 1] = hex[random[index] & 0x0f];
        }
        return token;
    }

    std::string Key(const std::string& message_id) const { return key_prefix_ + message_id; }

    std::unique_ptr<sw::redis::Redis> redis_;
    std::string key_prefix_;
    uint64_t completed_ttl_ms_ = 0;
    uint64_t processing_retention_ms_ = 0;
};

} // namespace oj::judge
