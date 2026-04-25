#pragma once
#include <cstdlib>
#include <string>

namespace ns_runtime_cfg
{
	inline std::string GetEnvOrDefault(const char* key, const std::string& fallback)
	{
		const char* value = std::getenv(key);
		if (value == nullptr)
		{
			return fallback;
		}
		std::string out(value);
		if (out.empty())
		{
			return fallback;
		}
		return out;
	}

	inline int GetEnvIntOrDefault(const char* key, int fallback)
	{
		const char* value = std::getenv(key);
		if (value == nullptr)
		{
			return fallback;
		}
		try
		{
			return std::stoi(value);
		}
		catch (...)
		{
			return fallback;
		}
	}

	inline int ParseIntOrDefault(const std::string& value, int fallback)
	{
		if (value.empty())
		{
			return fallback;
		}
		try
		{
			return std::stoi(value);
		}
		catch (...)
		{
			return fallback;
		}
	}
}

const std::string HTML_PATH = "/home/yupureki/project/OnlineJudge/src/wwwroot/";
const std::string LOG_PATH  = "/home/yupureki/project/OnlineJudge/log/";
const std::string CONF_PATH = "/home/yupureki/project/OnlineJudge/conf/";
const std::string oj_tests = ns_runtime_cfg::GetEnvOrDefault("OJ_TESTS", "tests");
const std::string oj_questions = ns_runtime_cfg::GetEnvOrDefault("OJ_QUESTIONS", "questions");
const std::string oj_users = ns_runtime_cfg::GetEnvOrDefault("OJ_USERS", "users");

const std::string host = ns_runtime_cfg::GetEnvOrDefault("OJ_DB_HOST", "localhost");
const std::string user = ns_runtime_cfg::GetEnvOrDefault("OJ_DB_USER", "oj_server");
const std::string passwd = ns_runtime_cfg::GetEnvOrDefault("OJ_DB_PASS", "password");
const std::string db = ns_runtime_cfg::GetEnvOrDefault("OJ_DB_NAME", "myoj");
const int port = ns_runtime_cfg::GetEnvIntOrDefault("OJ_DB_PORT", ns_runtime_cfg::ParseIntOrDefault("3306", 3306));

const std::string redis_addr = ns_runtime_cfg::GetEnvOrDefault("OJ_REDIS_ADDR", "");

const std::string smtp_host = ns_runtime_cfg::GetEnvOrDefault("OJ_SMTP_HOST", "smtp.qq.com");
const int smtp_port = ns_runtime_cfg::GetEnvIntOrDefault("OJ_SMTP_PORT", ns_runtime_cfg::ParseIntOrDefault("465", 465));
const std::string smtp_user = ns_runtime_cfg::GetEnvOrDefault("OJ_SMTP_USER", "2954585082@qq.com");
const std::string smtp_passwd = ns_runtime_cfg::GetEnvOrDefault("OJ_SMTP_PASS", "ggvcplcgbwbudcef");
const std::string smtp_from = ns_runtime_cfg::GetEnvOrDefault("OJ_SMTP_FROM", "2954585082@qq.com");
const bool smtp_ssl = ns_runtime_cfg::GetEnvOrDefault("OJ_SMTP_SSL", "true") == "true";

const std::string admin_host = ns_runtime_cfg::GetEnvOrDefault("OJ_ADMIN_HOST", "0.0.0.0");
const int admin_port = ns_runtime_cfg::GetEnvIntOrDefault("OJ_ADMIN_PORT", ns_runtime_cfg::ParseIntOrDefault("8090", 8090));
