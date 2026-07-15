#pragma once

#include "legacy_contract_manifest.hpp"

#include "biz_service.pb.h"

#include <google/protobuf/descriptor.h>

#include <iostream>
#include <set>
#include <string>
#include <string_view>

namespace oj::legacy_contract::test {

inline bool IsCanonicalError(std::string_view code) {
    for (const auto& error : kErrors) {
        if (error.code == code) return true;
    }
    return false;
}

class Checks {
public:
    void Expect(bool condition, std::string_view message) {
        if (!condition) {
            ++failures_;
            std::cerr << "FAIL: " << message << '\n';
        }
    }

    int Finish(std::string_view suite) const {
        if (failures_ != 0) {
            std::cerr << suite << ": " << failures_ << " contract assertion(s) failed\n";
            return 1;
        }
        std::cout << suite << ": manifest contract passed\n";
        return 0;
    }

private:
    int failures_ = 0;
};

template <std::size_t N>
void CheckRpcTable(Checks& checks, const std::array<RpcContract, N>& rpcs) {
    std::set<std::string> keys;
    for (const auto& rpc : rpcs) {
        checks.Expect(!rpc.domain.empty(), "RPC domain must be named");
        checks.Expect(!rpc.legacy_verb.empty() && !rpc.legacy_path.empty(), "legacy route must be named");
        checks.Expect(!rpc.service.empty() && rpc.service.find("Service") != std::string_view::npos,
                      "RPC service must use a protobuf Service name");
        checks.Expect(!rpc.method.empty(), "RPC method must be named");
        const std::string native_path = "/oj.biz." + std::string(rpc.service) + "/" + std::string(rpc.method);
        checks.Expect(native_path.rfind("/oj.biz.", 0) == 0 && native_path.find("/api/") == std::string::npos,
                      "RPC maps to a native /oj.biz.<Service>/<Method> path");
        checks.Expect(!rpc.request.empty() && rpc.request.find("Request") != std::string_view::npos,
                      "RPC request message must be explicit");
        checks.Expect(!rpc.response.empty() && rpc.response.find("Response") != std::string_view::npos,
                      "RPC response message must be explicit");
        const auto* service = google::protobuf::DescriptorPool::generated_pool()->FindServiceByName(
            "oj.biz." + std::string(rpc.service));
        checks.Expect(service != nullptr, "RPC service must exist in the generated protobuf contract");
        if (service != nullptr) {
            const auto* method = service->FindMethodByName(std::string(rpc.method));
            checks.Expect(method != nullptr, "RPC method must exist in the generated protobuf contract");
            if (method != nullptr) {
                checks.Expect(method->input_type()->name() == rpc.request,
                              "RPC request must match the generated protobuf method");
                checks.Expect(method->output_type()->name() == rpc.response,
                              "RPC response must match the generated protobuf method");
            }
        }
        checks.Expect(rpc.method.find("/api/") == std::string_view::npos,
                      "native RPC method must not preserve an /api path");
        std::string_view errors = rpc.legacy_errors;
        while (!errors.empty()) {
            const auto comma = errors.find(',');
            const auto code = errors.substr(0, comma);
            checks.Expect(IsCanonicalError(code), "RPC errors must use the canonical error vocabulary");
            if (comma == std::string_view::npos) break;
            errors.remove_prefix(comma + 1);
        }
        const std::string key = std::string(rpc.legacy_verb) + " " + std::string(rpc.legacy_path);
        checks.Expect(keys.insert(key).second, "legacy route mapping must be unique");
    }
}

template <std::size_t N>
bool HasDomain(const std::array<RpcContract, N>& rpcs, std::string_view domain) {
    for (const auto& rpc : rpcs) {
        if (rpc.domain == domain) return true;
    }
    return false;
}

template <std::size_t N>
bool HasMethod(const std::array<RpcContract, N>& rpcs, std::string_view service,
               std::string_view method) {
    for (const auto& rpc : rpcs) {
        if (rpc.service == service && rpc.method == method) return true;
    }
    return false;
}

inline void CheckSharedWireContract(Checks& checks) {
    checks.Expect(oj::biz::UserService::descriptor() != nullptr,
                  "generated business protobuf descriptors are linked");
    checks.Expect(kProtobufContentType == "application/x-protobuf",
                  "dynamic requests and responses use application/x-protobuf");
    checks.Expect(kUserCookie == "oj_session_id", "user cookie name is frozen");
    checks.Expect(kAdminCookie == "oj_admin_session_id", "admin cookie name is frozen");
    checks.Expect(kErrors.front().code == "OK" && kErrors.front().http_status == 200,
                  "success maps to HTTP 200");
    for (std::size_t i = 1; i < kErrors.size(); ++i) {
        checks.Expect(kErrors[i - 1].http_status < kErrors[i].http_status,
                      "canonical HTTP error mappings are ordered and unique");
    }
}

template <std::size_t N>
void CheckPaging(Checks& checks, const std::array<PagingContract, N>& paging) {
    for (const auto& item : paging) {
        checks.Expect(item.default_page == 1, "paging is one-based");
        checks.Expect(item.default_size > 0 && item.default_size <= item.max_size,
                      "default page size is positive and clamped");
        checks.Expect(item.max_page >= item.default_page && item.max_size >= item.default_size,
                      "paging bounds include defaults");
    }
}

}  // namespace oj::legacy_contract::test
