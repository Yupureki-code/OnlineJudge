#include "../include/jwt_middleware.hpp"
#include <algorithm>

namespace ns_gateway {

std::string JWTMiddleware::ExtractCookie(const std::string& cookie, const std::string& key) {
    std::string search = key + "=";
    auto pos = cookie.find(search);
    if (pos == std::string::npos) {
        return "";
    }
    pos += search.size();
    auto end = cookie.find(';', pos);
    if (end == std::string::npos) {
        return cookie.substr(pos);
    }
    return cookie.substr(pos, end - pos);
}

std::string JWTMiddleware::ExtractBearerToken(const std::string& auth_header) {
    const std::string prefix = "Bearer ";
    if (auth_header.compare(0, prefix.size(), prefix) != 0) {
        return "";
    }
    return auth_header.substr(prefix.size());
}

std::optional<ns_jwt::Claims> JWTMiddleware::Authenticate(
    const std::string& auth_header,
    const std::string& cookie) {

    std::string token;

    // 优先从 Authorization 头提取
    if (!auth_header.empty()) {
        token = ExtractBearerToken(auth_header);
    }

    // 回退到 Cookie
    if (token.empty() && !cookie.empty()) {
        token = ExtractCookie(cookie, "oj_session_id");
    }

    if (token.empty()) {
        return std::nullopt;
    }

    return ns_jwt::JWTManager::Instance().Verify(token);
}

} // namespace ns_gateway