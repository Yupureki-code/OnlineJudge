#pragma once
#include "../../comm/jwt.hpp"
#include <string>
#include <optional>

namespace ns_gateway {

/// JWT 中间件：在 Gateway 层验证 JWT，提取用户身份
class JWTMiddleware {
public:
    /// 从 HTTP 请求中提取并验证 JWT
    /// @param auth_header Authorization 头（Bearer xxx）
    /// @param cookie Cookie 字符串（可能包含 oj_session_id）
    /// @return 验证成功返回 Claims，失败返回 nullopt
    static std::optional<ns_jwt::Claims> Authenticate(
        const std::string& auth_header,
        const std::string& cookie);

    /// 从 Cookie 字符串中提取指定字段
    /// @param cookie Cookie 头
    /// @param key 字段名
    /// @return 字段值或空字符串
    static std::string ExtractCookie(
        const std::string& cookie,
        const std::string& key);

    /// 从 Authorization 头提取 Bearer token
    /// @param auth_header Authorization 头
    /// @return token 或空字符串
    static std::string ExtractBearerToken(const std::string& auth_header);
};

} // namespace ns_gateway