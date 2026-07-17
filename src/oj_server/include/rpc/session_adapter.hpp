#pragma once

#include <string>
#include <string_view>
#include <cstdlib>
#include <fstream>

#include <brpc/controller.h>
#include <butil/endpoint.h>
#include <openssl/crypto.h>

namespace oj::rpc
{

class SessionAdapter
{
public:
    static constexpr std::string_view kCookieName = "oj_session_id";
    //获取Cookie
    static std::string Cookie(const brpc::Controller& controller)
    {
        if (!controller.has_http_request())
            return {};
        const auto* value = controller.http_request().GetHeader("Cookie");
        return value == nullptr ? std::string{} : *value;
    }

    static std::string RemoteIp(const brpc::Controller& controller)
    {
        if (controller.has_http_request())
        {
            const std::string configured_token = GatewayToken();
            const auto* supplied_token = controller.http_request().GetHeader("X-OJ-Gateway-Token");
            const auto* client_ip = controller.http_request().GetHeader("X-OJ-Client-IP");
            if (!configured_token.empty() && supplied_token != nullptr &&
                client_ip != nullptr && !client_ip->empty())
            {
                if (configured_token.size() == supplied_token->size() &&
                    CRYPTO_memcmp(configured_token.data(), supplied_token->data(), configured_token.size()) == 0)
                {
                    return *client_ip;
                }
            }
        }

        const auto endpoint = controller.remote_side();
        if (!butil::is_endpoint_extended(endpoint))
            return butil::ip2str(endpoint.ip).c_str();
        std::string text = butil::endpoint2str(endpoint).c_str();
        if (!text.empty() && text.front() == '[') {
            const std::size_t close = text.find(']');
            if (close != std::string::npos)
                return text.substr(1, close - 1);
        }
        return text;
    }

    static std::string GatewayToken()
    {
        const char* token_file = std::getenv("OJ_GATEWAY_AUTH_TOKEN_FILE");
        if (token_file != nullptr && *token_file != '\0')
        {
            std::ifstream input(token_file);
            std::string token;
            if (input && std::getline(input, token))
            {
                while (!token.empty() && (token.back() == '\r' || token.back() == '\n')) token.pop_back();
                if (!token.empty()) return token;
            }
        }
        const char* token = std::getenv("OJ_GATEWAY_AUTH_TOKEN");
        return token == nullptr ? std::string{} : std::string(token);
    }

    template <typename Control, typename User>
    static bool GetSessionUser(const brpc::Controller& controller,
                               Control& control,
                               User* user)
    {
        return control.GetSessionUser(Cookie(controller), user);
    }

    template <typename Control>
    static std::string CreateSession(brpc::Controller& controller,
                                     Control& control,
                                     int user_id,
                                     const std::string& email)
    {
        std::string session_id = control.CreateSession(user_id, email);
        if (!session_id.empty())
            controller.http_response().SetHeader("Set-Cookie", control.GetSetCookieHeader(session_id));
        return session_id;
    }

    template <typename Control>
    static bool DestroySession(brpc::Controller& controller, Control& control)
    {
        const std::string session_id = CookieValue(Cookie(controller), kCookieName);
        if (!session_id.empty() && !control.DestroySession(session_id))
            return false;
        controller.http_response().SetHeader("Set-Cookie", control.GetClearCookieHeader());
        return true;
    }

    static std::string CookieValue(std::string_view cookie_header, std::string_view name)
    {
        std::size_t start = 0;
        while (start < cookie_header.size()) {
            const std::size_t end = cookie_header.find(';', start);
            std::string_view item = cookie_header.substr(
                start, end == std::string_view::npos ? cookie_header.size() - start : end - start);
            const std::size_t first = item.find_first_not_of(" \t");
            if (first != std::string_view::npos) {
                item.remove_prefix(first);
                const std::size_t equals = item.find('=');
                if (equals != std::string_view::npos && item.substr(0, equals) == name)
                    return std::string(item.substr(equals + 1));
            }
            if (end == std::string_view::npos)
                break;
            start = end + 1;
        }
        return {};
    }
};

class AdminSessionAdapter
{
public:
    static constexpr std::string_view kCookieName = "oj_admin_session_id";

    template <typename Store, typename Session>
    static bool GetSession(const brpc::Controller& controller, Store& store, Session* session)
    {
        return store.GetSessionByCookie(SessionAdapter::Cookie(controller), session);
    }

    template <typename Store, typename Admin, typename User>
    static std::string CreateSession(brpc::Controller& controller,
                                     Store& store,
                                     const Admin& admin,
                                     const User& user)
    {
        std::string session_id = store.CreateSession(admin, user);
        if (!session_id.empty())
            controller.http_response().SetHeader("Set-Cookie", store.BuildSetCookieHeader(session_id));
        return session_id;
    }

    template <typename Store>
    static bool DestroySession(brpc::Controller& controller, Store& store)
    {
        if (!store.DestroySessionByCookie(SessionAdapter::Cookie(controller)))
            return false;
        controller.http_response().SetHeader("Set-Cookie", store.BuildClearCookieHeader());
        return true;
    }
};

} // namespace oj::rpc
