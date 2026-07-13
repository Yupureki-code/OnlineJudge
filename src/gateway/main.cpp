// src/gateway/main.cpp
// brpc Gateway 服务入口
//
// 职责：
// 1. 接收浏览器 HTTP/JSON 请求
// 2. JWT 验证
// 3. 路由分发到 Business Service 或 Judge Service
// 4. JSON ↔ Protobuf 转换
// 5. 静态文件服务

#include <brpc/server.h>
#include <butil/logging.h>
#include <butil/string_splitter.h>
#include <jsoncpp/json/json.h>
#include <string>
#include <memory>
#include <unordered_map>

#include "include/router.hpp"
#include "include/jwt_middleware.hpp"
#include "include/json_converter.hpp"
#include "include/rate_limiter.hpp"
#include "../comm/jwt.hpp"

namespace ns_gateway {

/// Gateway HTTP 服务实现
class GatewayServiceImpl : public brpc::HttpService {
public:
    GatewayServiceImpl() {
        // 初始化 JWT
        ns_jwt::JWTConfig jwt_cfg;
        jwt_cfg.secret = std::getenv("OJ_JWT_SECRET") ? std::getenv("OJ_JWT_SECRET") : "default_secret";
        jwt_cfg.ttl_seconds = 604800;  // 7天
        jwt_cfg.issuer = "oj-gateway";
        ns_jwt::JWTManager::Instance().Init(jwt_cfg);

        // 加载路由表
        const char* conf_path = std::getenv("OJ_GATEWAY_CONF");
        if (!conf_path) conf_path = "conf/gateway.conf";
        if (!_router.LoadFromFile(conf_path)) {
            LOG(WARNING) << "Failed to load gateway config from " << conf_path
                         << ", using defaults";
            LoadDefaultRoutes();
        }

        // 限流器：100 请求/分钟/IP
        _rate_limiter = std::make_unique<RateLimiter>(100, 60);
    }

    void default_method(::google::protobuf::RpcController* cntl_base,
                       const brpc::HttpRequest* request,
                       brpc::HttpResponse* response,
                       ::google::protobuf::Closure* done) override {
        brpc::ClosureGuard done_guard(done);
        brpc::Controller* cntl = static_cast<brpc::Controller*>(cntl_base);

        const std::string path = cntl->http_request().uri().path();
        LOG(INFO) << "Gateway received: " << cntl->http_request().method()
                  << " " << path;

        // 限流检查
        std::string client_ip = butil::endpoint2str(cntl->remote_side()).c_str();
        if (!_rate_limiter->Allow(client_ip)) {
            cntl->http_response().set_status_code(brpc::HTTP_STATUS_TOO_MANY_REQUESTS);
            cntl->http_response().set_content_type("application/json");
            cntl->response_attachment() =
                "{\"success\":false,\"error_code\":\"RATE_LIMITED\"}";
            return;
        }

        // 健康检查
        if (path == "/api/ping") {
            cntl->http_response().set_content_type("application/json");
            cntl->response_attachment() =
                "{\"status\":\"ok\",\"version\":\"0.1.0\"}";
            return;
        }

        // 路由匹配
        const RouteEntry* route = _router.Match(path);
        if (!route) {
            cntl->http_response().set_status_code(brpc::HTTP_STATUS_NOT_FOUND);
            cntl->http_response().set_content_type("application/json");
            cntl->response_attachment() =
                "{\"success\":false,\"error_code\":\"NOT_FOUND\"}";
            return;
        }

        // 认证检查
        if (route->require_auth) {
            const std::string* ah = cntl->http_request().GetHeader("Authorization");
            const std::string* ck = cntl->http_request().GetHeader("Cookie");
            std::string auth_header = ah ? *ah : "";
            std::string cookie = ck ? *ck : "";
            auto claims = JWTMiddleware::Authenticate(auth_header, cookie);

            if (!claims.has_value()) {
                cntl->http_response().set_status_code(brpc::HTTP_STATUS_UNAUTHORIZED);
                cntl->http_response().set_content_type("application/json");
                cntl->response_attachment() =
                    "{\"success\":false,\"error_code\":\"UNAUTHORIZED\"}";
                return;
            }

            // 管理员权限检查
            if (route->require_admin && claims->role < 1) {
                cntl->http_response().set_status_code(brpc::HTTP_STATUS_FORBIDDEN);
                cntl->http_response().set_content_type("application/json");
                cntl->response_attachment() =
                    "{\"success\":false,\"error_code\":\"FORBIDDEN\"}";
                return;
            }

            // 注入 user_id 到 brpc 头（透传给后端服务）
            ns_jwt::InjectUserIdToHeader(cntl, claims->user_id);
        }

        // 转发请求到后端
        ForwardToBackend(cntl, route->backend_method, route->backend_service);
    }

    const std::vector<RouteEntry>& GetRoutes() const { return _router.GetRoutes(); }

private:
    Router _router;
    std::unique_ptr<RateLimiter> _rate_limiter;
    std::unordered_map<std::string, std::unique_ptr<brpc::Channel>> _channels;

    std::string GetBackendUrl(const std::string& service) {
        const char* env = std::getenv("OJ_BUSINESS_ADDR");
        if (service == "business") return env ? env : "http://127.0.0.1:8081";
        return "http://127.0.0.1:8082";
    }

    brpc::Channel* GetOrCreateChannel(const std::string& service) {
        auto it = _channels.find(service);
        if (it != _channels.end()) return it->second.get();
        auto ch = std::make_unique<brpc::Channel>();
        brpc::ChannelOptions opts;
        opts.timeout_ms = 30000;
        opts.protocol = brpc::PROTOCOL_HTTP;
        if (ch->Init(GetBackendUrl(service).c_str(), &opts) != 0) return nullptr;
        _channels[service] = std::move(ch);
        return _channels[service].get();
    }

    void ForwardToBackend(brpc::Controller* cntl, const std::string& method, const std::string& service) {
        // CORS 预检
        if (cntl->http_request().method() == brpc::HTTP_METHOD_OPTIONS) {
            cntl->http_response().SetHeader("Access-Control-Allow-Origin", "*");
            cntl->http_response().SetHeader("Access-Control-Allow-Methods", "GET,POST,PUT,DELETE,OPTIONS");
            cntl->http_response().SetHeader("Access-Control-Allow-Headers", "Content-Type,Authorization");
            cntl->http_response().set_status_code(brpc::HTTP_STATUS_OK);
            return;
        }

        brpc::Channel* ch = GetOrCreateChannel(service);
        if (!ch) {
            cntl->http_response().set_content_type("application/json");
            cntl->response_attachment() = "{\"success\":false,\"error_code\":\"BACKEND_UNREACHABLE\"}";
            return;
        }

        cntl->http_response().SetHeader("Access-Control-Allow-Origin", "*");

        brpc::Controller backend_cntl;
        backend_cntl.http_request().set_method(cntl->http_request().method());
        backend_cntl.http_request().uri() = cntl->http_request().uri().path();
        backend_cntl.request_attachment() = cntl->request_attachment();

        ch->CallMethod(nullptr, &backend_cntl, nullptr, nullptr, nullptr);

        if (backend_cntl.Failed()) {
            cntl->http_response().set_content_type("application/json");
            cntl->response_attachment() = "{\"success\":false,\"error_code\":\"BACKEND_ERROR\"}";
            return;
        }
        cntl->http_response().set_content_type("application/json");
        cntl->response_attachment() = backend_cntl.response_attachment();
    }

    void LoadDefaultRoutes() {
        _router.GetRoutes() = std::vector<RouteEntry>{
            {"/api/auth",       "business", "", false, false},
            {"/api/user",       "business", "", true,  false},
            {"/api/question",   "business", "", false, false},
            {"/api/solution",   "business", "", false, false},
            {"/api/comment",    "business", "", true,  false},
            {"/api/admin",      "business", "", true,  true},
            {"/api/judge",      "judge",     "", true,  false},
        };
    }
};

} // namespace ns_gateway

// ==================== main ====================

int main(int argc, char* argv[]) {
    gflags::ParseCommandLineFlags(&argc, &argv, true);

    brpc::Server server;
    ns_gateway::GatewayServiceImpl gateway;

    if (server.AddService(&gateway, brpc::SERVER_DOESNT_OWN_SERVICE) != 0) {
        LOG(ERROR) << "Failed to add gateway service";
        return -1;
    }

    brpc::ServerOptions options;
    int port = 8080;
    if (argc > 1) {
        port = std::atoi(argv[1]);
    }

    if (server.Start(port, &options) != 0) {
        LOG(ERROR) << "Failed to start gateway on port " << port;
        return -1;
    }

    LOG(INFO) << "Gateway running on port " << port;
    LOG(INFO) << "Routes loaded: " << gateway.GetRoutes().size();
    server.RunUntilAskedToQuit();

    return 0;
}