#pragma once
#include <jwt-cpp/jwt.h>
#include <string>
#include <optional>
#include <cstdint>
#include <chrono>
#include <random>
#include <sstream>
#include <stdexcept>

namespace ns_jwt {

/// JWT Claims（载荷）
struct Claims {
    uint32_t user_id;
    std::string email;
    int32_t role;          // 0=普通用户, 1=管理员, 2=超管
    std::string jti;
    int64_t exp;
};

/// JWT 配置
struct JWTConfig {
    std::string secret;
    int64_t ttl_seconds = 604800;  // 7天
    std::string issuer = "oj-gateway";
};

/// JWT 管理器
class JWTManager {
public:
    static JWTManager& Instance() { static JWTManager i; return i; }

    void Init(const JWTConfig& config) { _config = config; _initialized = true; }

    std::string Sign(uint32_t user_id, const std::string& email, int32_t role) {
        if (!_initialized) throw std::runtime_error("JWTManager not initialized");

        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, 15);
        std::stringstream jti_ss;
        jti_ss << std::hex;
        for (int i = 0; i < 32; ++i) jti_ss << dis(gen);
        std::string jti = jti_ss.str();

        auto now = std::chrono::system_clock::now();
        auto exp_time = now + std::chrono::seconds(_config.ttl_seconds);

        return jwt::create()
            .set_issuer(_config.issuer)
            .set_type("JWT")
            .set_issued_at(now)
            .set_expires_at(exp_time)
            .set_payload_claim("user_id", jwt::claim(std::to_string(user_id)))
            .set_payload_claim("email", jwt::claim(email))
            .set_payload_claim("role", jwt::claim(std::to_string(role)))
            .set_id(jti)
            .sign(jwt::algorithm::hs256{_config.secret});
    }

    std::optional<Claims> Verify(const std::string& token) {
        if (!_initialized) return std::nullopt;
        try {
            auto decoded = jwt::decode(token);
            auto verifier = jwt::verify()
                .allow_algorithm(jwt::algorithm::hs256{_config.secret})
                .with_issuer(_config.issuer);
            verifier.verify(decoded);

            std::string jti = decoded.get_id();
            if (IsBlacklisted(jti)) return std::nullopt;

            Claims claims;
            claims.user_id = std::stoul(decoded.get_payload_claim("user_id").as_string());
            claims.email = decoded.get_payload_claim("email").as_string();
            claims.role = std::stoi(decoded.get_payload_claim("role").as_string());
            claims.jti = jti;
            auto exp = decoded.get_expires_at();
            claims.exp = std::chrono::duration_cast<std::chrono::seconds>(
                exp.time_since_epoch()).count();
            return claims;
        } catch (const std::exception&) {
            return std::nullopt;
        }
    }

    bool IsBlacklisted(const std::string& jti) {
        // TODO: 接入 Redis 黑名单查询
        return false;
    }

    void AddToBlacklist(const std::string& jti, int64_t ttl_remaining) {
        // TODO: 接入 Redis 黑名单写入
    }

private:
    JWTManager() = default;
    JWTConfig _config;
    bool _initialized = false;
};

/// brpc 头注入/提取（弱符号，链接 brpc 后强符号覆盖）
__attribute__((weak))
inline uint32_t ExtractUserIdFromHeader(void* cntl) { return 0; }

__attribute__((weak))
inline void InjectUserIdToHeader(void* cntl, uint32_t user_id) {}

} // namespace ns_jwt