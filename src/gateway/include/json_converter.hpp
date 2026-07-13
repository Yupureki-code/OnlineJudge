#pragma once
#include <string>
#include <google/protobuf/message.h>

namespace ns_gateway {

/// JSON ↔ Protobuf 转换器
///
/// Gateway 接收浏览器 JSON 请求，转换为 Protobuf 发给后端服务
/// 后端返回 Protobuf 响应，转换为 JSON 返回前端
class JsonConverter {
public:
    /// JSON 字符串 → Protobuf 消息
    /// @param json_str JSON 字符串
    /// @param msg 目标 Protobuf 消息
    /// @return true=成功
    static bool FromJson(const std::string& json_str, google::protobuf::Message& msg);

    /// Protobuf 消息 → JSON 字符串
    /// @param msg Protobuf 消息
    /// @return JSON 字符串
    static std::string ToJson(const google::protobuf::Message& msg);

    /// Protobuf 消息 → JSON 字符串（带缩进，调试用）
    static std::string ToJsonPretty(const google::protobuf::Message& msg);
};

} // namespace ns_gateway