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

	inline std::string GetPathOrDefault(const char* key, const std::string& fallback)
	{
		std::string path = GetEnvOrDefault(key, fallback);
		if (!path.empty() && path.back() != '/')
		{
			path.push_back('/');
		}
		return path;
	}
}

const std::string HTML_PATH = ns_runtime_cfg::GetPathOrDefault("OJ_HTML_PATH", "@HTML_PATH@");
const std::string LOG_PATH  = ns_runtime_cfg::GetPathOrDefault("OJ_LOG_PATH", "@LOG_PATH@");
const std::string CONF_PATH = ns_runtime_cfg::GetPathOrDefault("OJ_CONF_PATH", "@CONF_PATH@");
const std::string oj_tests = ns_runtime_cfg::GetEnvOrDefault("OJ_TESTS", "@OJ_TESTS@");
const std::string oj_questions = ns_runtime_cfg::GetEnvOrDefault("OJ_QUESTIONS", "@OJ_QUESTIONS@");
const std::string oj_users = ns_runtime_cfg::GetEnvOrDefault("OJ_USERS", "@OJ_USERS@");

const std::string host = ns_runtime_cfg::GetEnvOrDefault("OJ_DB_HOST", "@HOST@");
const std::string user = ns_runtime_cfg::GetEnvOrDefault("OJ_DB_USER", "@USER@");
const std::string passwd = ns_runtime_cfg::GetEnvOrDefault("OJ_DB_PASSWD", "@PASSWD@");
const std::string db = ns_runtime_cfg::GetEnvOrDefault("OJ_DB_NAME", "@MYOJ@");
const int port = ns_runtime_cfg::GetEnvIntOrDefault("OJ_DB_PORT", ns_runtime_cfg::ParseIntOrDefault("@PORT@", 3306));

const std::string redis_addr = ns_runtime_cfg::GetEnvOrDefault("OJ_REDIS_ADDR", "@REDIS@");

const std::string smtp_host = ns_runtime_cfg::GetEnvOrDefault("OJ_SMTP_HOST", "@SMTP_HOST@");
const int smtp_port = ns_runtime_cfg::GetEnvIntOrDefault("OJ_SMTP_PORT", ns_runtime_cfg::ParseIntOrDefault("@SMTP_PORT@", 465));
const std::string smtp_user = ns_runtime_cfg::GetEnvOrDefault("OJ_SMTP_USER", "@SMTP_USER@");
const std::string smtp_passwd = ns_runtime_cfg::GetEnvOrDefault("OJ_SMTP_PASSWD", "@SMTP_PASSWD@");
const std::string smtp_from = ns_runtime_cfg::GetEnvOrDefault("OJ_SMTP_FROM", "@SMTP_FROM@");
const bool smtp_ssl = ns_runtime_cfg::GetEnvOrDefault("OJ_SMTP_SSL", "@SMTP_SSL@") == "true";

const std::string admin_host = ns_runtime_cfg::GetEnvOrDefault("OJ_ADMIN_HOST", "@ADMIN_HOST@");
const int admin_port = ns_runtime_cfg::GetEnvIntOrDefault("OJ_ADMIN_PORT", ns_runtime_cfg::ParseIntOrDefault("@ADMIN_PORT@", 8090));

const std::string es_host = ns_runtime_cfg::GetEnvOrDefault("OJ_ES_HOST", "@ES_HOST@");
const std::string es_passwd = ns_runtime_cfg::GetEnvOrDefault("OJ_ES_PASSWD", "@ES_PASSWD@");
const std::string es_user = ns_runtime_cfg::GetEnvOrDefault("OJ_ES_USER", "@ES_USER@");
