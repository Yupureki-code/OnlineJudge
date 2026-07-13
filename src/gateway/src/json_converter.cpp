#include "../include/json_converter.hpp"
#include <google/protobuf/util/json_util.h>

namespace ns_gateway {

bool JsonConverter::FromJson(const std::string& json_str, google::protobuf::Message& msg) {
    google::protobuf::util::JsonParseOptions options;
    options.ignore_unknown_fields = true;
    auto status = google::protobuf::util::JsonStringToMessage(json_str, &msg, options);
    return status.ok();
}

std::string JsonConverter::ToJson(const google::protobuf::Message& msg) {
    std::string output;
    google::protobuf::util::JsonPrintOptions options;
    options.always_print_primitive_fields = false;
    auto status = google::protobuf::util::MessageToJsonString(msg, &output, options);
    if (!status.ok()) {
        return "{}";
    }
    return output;
}

std::string JsonConverter::ToJsonPretty(const google::protobuf::Message& msg) {
    std::string output;
    google::protobuf::util::JsonPrintOptions options;
    options.add_whitespace = true;
    options.always_print_primitive_fields = true;
    auto status = google::protobuf::util::MessageToJsonString(msg, &output, options);
    if (!status.ok()) {
        return "{}";
    }
    return output;
}

} // namespace ns_gateway