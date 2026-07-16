#pragma once

#include "../../comm/comm.hpp"
#include <httplib.h>
#include <cctype>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>

class JsonUtil
{
public:
    static bool ParseJsonBody(const httplib::Request& request, Json::Value* output)
    {
        if (output == nullptr)
            return false;
        Json::CharReaderBuilder builder;
        std::istringstream stream(request.body);
        return Json::parseFromStream(builder, stream, output, nullptr);
    }

    static bool ExtractAndValidateEmail(const Json::Value& input, std::string* email)
    {
        if (email == nullptr || !input.isMember("email") || !input["email"].isString())
            return false;
        *email = oj::util::StringUtil::Trim(input["email"].asString());
        return oj::util::StringUtil::IsValidEmail(*email);
    }

    static bool ExtractAndValidatePassword(const Json::Value& input, std::string* password)
    {
        if (password == nullptr || !input.isMember("password") || !input["password"].isString())
            return false;
        *password = input["password"].asString();
        return !password->empty();
    }
};

class HttpUtil
{
public:
    static int ParsePositiveIntParam(const httplib::Request& request, const std::string& name,
                                     int fallback, int minimum, int maximum)
    {
        if (!request.has_param(name))
            return fallback;
        try
        {
            const std::string value = request.get_param_value(name);
            size_t parsed = 0;
            const long long number = std::stoll(value, &parsed);
            if (parsed != value.size() || number < minimum || number > maximum)
                return fallback;
            return static_cast<int>(number);
        }
        catch (...)
        {
            return fallback;
        }
    }

    static std::string url_encode(const std::string& value)
    {
        std::ostringstream encoded;
        encoded << std::uppercase << std::hex;
        for (unsigned char ch : value)
        {
            if (std::isalnum(ch) || ch == '-' || ch == '_' || ch == '.' || ch == '~')
            {
                encoded << static_cast<char>(ch);
            }
            else
            {
                encoded << '%' << std::setw(2) << std::setfill('0') << static_cast<int>(ch);
            }
        }
        return encoded.str();
    }

    static void ReplyBadRequest(httplib::Response& response, const std::string& message)
    {
        Json::Value body;
        body["success"] = false;
        body["message"] = message;
        Json::FastWriter writer;
        response.status = 400;
        response.set_content(writer.write(body), "application/json;charset=utf-8");
    }
};

namespace oj::util
{
    class FileUtil
    {
    public:
        static bool ReadFile(const std::string& path, std::string* content)
        {
            if (content == nullptr)
                return false;
            std::ifstream input(path, std::ios::binary);
            if (!input.is_open())
                return false;
            std::ostringstream buffer;
            buffer << input.rdbuf();
            if (input.bad())
                return false;
            *content = buffer.str();
            return true;
        }

        static bool WriteFile(const std::string& path, const std::string& content)
        {
            std::ofstream output(path, std::ios::binary | std::ios::trunc);
            if (!output.is_open())
                return false;
            output.write(content.data(), static_cast<std::streamsize>(content.size()));
            return output.good();
        }
    };
}
