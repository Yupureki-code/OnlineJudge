// src/gateway/src/jwt_brpc.cpp
// JWT brpc 头注入/提取实现（链接 brpc 的 target）
#include "../include/jwt_middleware.hpp"
#include <brpc/controller.h>

namespace ns_jwt {

/// 从 brpc Controller 请求头提取 user_id（强符号覆盖弱符号）
uint32_t ExtractUserIdFromHeader(void* cntl_ptr) {
    auto* cntl = static_cast<brpc::Controller*>(cntl_ptr);
    const std::string* val = cntl->http_request().GetHeader("X-User-Id");
    if (!val || val->empty()) return 0;
    return static_cast<uint32_t>(std::stoul(*val));
}

/// 将 user_id 注入 brpc Controller 请求头
void InjectUserIdToHeader(void* cntl_ptr, uint32_t user_id) {
    auto* cntl = static_cast<brpc::Controller*>(cntl_ptr);
    cntl->http_request().SetHeader("X-User-Id", std::to_string(user_id));
}

} // namespace ns_jwt
