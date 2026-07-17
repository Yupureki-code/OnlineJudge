#pragma once

#include <string>
#include <mutex>
#include <unordered_map>
#include <array>
#include <charconv>
#include <utility>
#include <cstdlib>
#include <ctime>
#include <openssl/rand.h>
#include "control/oj_control.hpp"
#include "../../comm/comm.hpp"
#include "../../comm/logger.hpp"
#include "../../comm/models/admin.hxx"
#include "../../comm/models/user.hxx"

namespace oj::admin
{
	using namespace oj::util;
	using namespace oj::db;
	using namespace oj::logger;
	constexpr const char* kAdminVersion = "v0.2.0";
	constexpr const char* kAdminSessionCookieName = "oj_admin_session_id";
	constexpr const char* kAdminSessionPrefix = "oj:prod:v1:session:admin:";
	constexpr int kAdminSessionExpireSeconds = 60 * 60;

	struct AdminSession
	{
		std::string session_id;
		int admin_id = 0;
		int uid = 0;
		std::string name;
		std::string email;
		std::string role;
		time_t create_time = 0;
		time_t last_access_time = 0;
		time_t expires_at = 0;
	};
	class AdminSessionStore
	{
		private:
			enum class RedisLoadResult
			{
				Hit,
				Miss,
				Outage,
				Invalid
			};

	public:
		explicit AdminSessionStore(bool redis_enabled = !redis_addr.empty(),
		                           std::string redis_address = redis_addr)
			: _redis_enabled(redis_enabled), _redis_address(std::move(redis_address))
		{}

		std::string CreateSession(const AdminAccount& admin, const User& user)
		{
			AdminSession session;
			session.session_id = GenerateSessionId();
			if (session.session_id.empty())
			{
				return {};
			}
			session.admin_id = admin.admin_id;
			session.uid = admin.uid;
			session.name = user.name;
			session.email = user.email;
			session.role = admin.role;
			session.create_time = time(nullptr);
			session.last_access_time = session.create_time;
			session.expires_at = session.create_time + kAdminSessionExpireSeconds;

			if (_redis_enabled && !StoreInRedis(session))
			{
				return {};
			}
			StoreInMemory(session);
			return session.session_id;
		}

		bool GetSessionByCookie(const std::string& cookie_header, AdminSession* session)
		{
			if (session == nullptr)
			{
				return false;
			}
			std::string session_id;
			if (!ParseCookie(cookie_header, &session_id))
			{
				return false;
			}
			AdminSession loaded;
			if (_redis_enabled)
			{
				if (LoadFromRedis(session_id, &loaded) != RedisLoadResult::Hit)
				{
					RemoveFromMemory(session_id);
					return false;
				}
				StoreInMemory(loaded);
				*session = loaded;
				return true;
			}
			if (LoadFromMemory(session_id, &loaded))
			{
				*session = loaded;
				return true;
			}

			return false;
		}

		bool DestroySessionByCookie(const std::string& cookie_header)
		{
			std::string session_id;
			if (!ParseCookie(cookie_header, &session_id))
			{
				return true;
			}

			RemoveFromMemory(session_id);
			return DestroyInRedis(session_id);
		}

		std::string BuildSetCookieHeader(const std::string& session_id) const
		{
			time_t expire = time(nullptr) + kAdminSessionExpireSeconds;
			char expire_str[64];
			struct tm tm_info;
			gmtime_r(&expire, &tm_info);
			strftime(expire_str, sizeof(expire_str), "%a, %d %b %Y %H:%M:%S GMT", &tm_info);
			const bool secure = std::getenv("OJ_COOKIE_SECURE") != nullptr &&
				std::string(std::getenv("OJ_COOKIE_SECURE")) == "true";
			return std::string(kAdminSessionCookieName) + "=" + session_id +
				"; Expires=" + expire_str +
				"; Path=/admin; HttpOnly; SameSite=Strict" + (secure ? "; Secure" : "");
		}

		std::string BuildClearCookieHeader() const
		{
			const bool secure = std::getenv("OJ_COOKIE_SECURE") != nullptr &&
				std::string(std::getenv("OJ_COOKIE_SECURE")) == "true";
			return std::string(kAdminSessionCookieName) +
				"=; Expires=Thu, 01 Jan 1970 00:00:00 GMT; Path=/admin; HttpOnly; SameSite=Strict" +
				(secure ? "; Secure" : "");
		}

		private:
			template <typename Integer>
			static bool ParseInteger(const std::string& encoded, Integer* value)
			{
				if (value == nullptr || encoded.empty())
				{
					return false;
				}
				Integer parsed{};
				const char* begin = encoded.data();
				const char* end = begin + encoded.size();
				const auto result = std::from_chars(begin, end, parsed);
				if (result.ec != std::errc{} || result.ptr != end)
				{
					return false;
				}
				*value = parsed;
				return true;
			}

			std::mutex& MemoryMutex()
			{
				static std::mutex mtx;
				return mtx;
			}

			std::unordered_map<std::string, AdminSession>& MemorySessions()
			{
				static std::unordered_map<std::string, AdminSession> sessions;
				return sessions;
			}

			void StoreInMemory(const AdminSession& session)
			{
				std::unique_lock<std::mutex> lock(MemoryMutex());
				auto& memory_sessions = MemorySessions();
				memory_sessions[session.session_id] = session;
			}

			bool LoadFromMemory(const std::string& session_id, AdminSession* session)
			{
				std::unique_lock<std::mutex> lock(MemoryMutex());
				auto& memory_sessions = MemorySessions();
				auto it = memory_sessions.find(session_id);
				if (it == memory_sessions.end())
				{
					return false;
				}
				time_t now = time(nullptr);
				if (it->second.expires_at <= now)
				{
					memory_sessions.erase(it);
					return false;
				}
				it->second.last_access_time = now;
				it->second.expires_at = now + kAdminSessionExpireSeconds;
				*session = it->second;
				return true;
			}

			void RemoveFromMemory(const std::string& session_id)
			{
				std::unique_lock<std::mutex> lock(MemoryMutex());
				auto& memory_sessions = MemorySessions();
				memory_sessions.erase(session_id);
			}

			bool StoreInRedis(const AdminSession& session)
			{
				if (!_redis_enabled)
				{
					return true;
				}

				std::unordered_map<std::string, std::string> fields;
				fields["admin_id"] = std::to_string(session.admin_id);
				fields["uid"] = std::to_string(session.uid);
				fields["name"] = session.name;
				fields["email"] = session.email;
				fields["role"] = session.role;
				fields["create_time"] = std::to_string(static_cast<long long>(session.create_time));
				fields["last_access_time"] = std::to_string(static_cast<long long>(session.last_access_time));

				std::string key = BuildKey(session.session_id);
				try
				{
					sw::redis::Redis redis(_redis_address);
					redis.hmset(key, fields.begin(), fields.end());
					redis.expire(key, kAdminSessionExpireSeconds);
					return true;
				}
				catch (const std::exception& ex)
				{
					LOG_WARNING("{}{}", "admin redis create session failed: ", ex.what());
					return false;
				}
				catch (...)
				{
					LOG_WARNING("{}", "admin redis create session failed: unknown exception");
					return false;
				}
			}

			RedisLoadResult LoadFromRedis(const std::string& session_id, AdminSession* session)
			{
				if (!_redis_enabled || session == nullptr)
				{
					return RedisLoadResult::Invalid;
				}
				std::unordered_map<std::string, std::string> fields;
				std::string key = BuildKey(session_id);

				try
				{
					sw::redis::Redis redis(_redis_address);
					redis.hgetall(key, std::inserter(fields, fields.begin()));
				}
				catch (const std::exception& ex)
				{
					LOG_WARNING("{}{}", "admin redis hgetall failed: ", ex.what());
					return RedisLoadResult::Outage;
				}
				catch (...)
				{
					LOG_WARNING("{}", "admin redis hgetall failed: unknown exception");
					return RedisLoadResult::Outage;
				}
				if (fields.empty())
				{
					return RedisLoadResult::Miss;
				}

				auto admin_id_it = fields.find("admin_id");
				auto uid_it = fields.find("uid");
				auto name_it = fields.find("name");
				auto email_it = fields.find("email");
				auto role_it = fields.find("role");
				auto create_time_it = fields.find("create_time");
				auto last_access_time_it = fields.find("last_access_time");
				if (fields.size() != 7 || admin_id_it == fields.end() || uid_it == fields.end() ||
					name_it == fields.end() || email_it == fields.end() || role_it == fields.end() ||
					create_time_it == fields.end() || last_access_time_it == fields.end())
				{
					return RedisLoadResult::Invalid;
				}
				time_t now = time(nullptr);
				session->session_id = session_id;
				long long parsed_create_time = 0;
				long long parsed_last_access_time = 0;
				if (!ParseInteger(admin_id_it->second, &session->admin_id) || session->admin_id <= 0 ||
					!ParseInteger(uid_it->second, &session->uid) || session->uid <= 0 ||
					!ParseInteger(create_time_it->second, &parsed_create_time) || parsed_create_time <= 0 ||
					!ParseInteger(last_access_time_it->second, &parsed_last_access_time) ||
					parsed_last_access_time < parsed_create_time || name_it->second.empty() ||
					name_it->second.size() > 64 || email_it->second.empty() || email_it->second.size() > 320 ||
					(role_it->second != "admin" && role_it->second != "super_admin"))
				{
					LOG_WARNING("{}", "invalid admin Redis session hash rejected");
					return RedisLoadResult::Invalid;
				}
				session->name = name_it->second;
				session->email = email_it->second;
				session->role = role_it->second;
				session->create_time = static_cast<time_t>(parsed_create_time);
				session->last_access_time = now;
				session->expires_at = now + kAdminSessionExpireSeconds;

				try
				{
					sw::redis::Redis redis(_redis_address);
					redis.hset(key, "last_access_time", std::to_string(static_cast<long long>(session->last_access_time)));
					redis.expire(key, kAdminSessionExpireSeconds);
				}
				catch (const std::exception& ex)
				{
					LOG_WARNING("{}{}", "admin redis touch session failed: ", ex.what());
					return RedisLoadResult::Outage;
				}
				catch (...)
				{
					LOG_WARNING("{}", "admin redis touch session failed: unknown exception");
					return RedisLoadResult::Outage;
				}
				return RedisLoadResult::Hit;
			}

			bool DestroyInRedis(const std::string& session_id)
			{
				if (!_redis_enabled)
				{
					return true;
				}
				try
				{
					sw::redis::Redis redis(_redis_address);
					redis.del(BuildKey(session_id));
					return true;
				}
				catch (const std::exception& ex)
				{
					LOG_WARNING("{}{}", "admin redis destroy session failed: ", ex.what());
					return false;
				}
				catch (...)
				{
					LOG_WARNING("{}", "admin redis destroy session failed: unknown exception");
					return false;
				}
			}

			std::string GenerateSessionId() const
			{
				std::array<unsigned char, 32> bytes{};
				if (RAND_bytes(bytes.data(), bytes.size()) != 1)
				{
					LOG_ERROR("{}", "CSPRNG failed while creating admin session");
					return {};
				}
				static constexpr char hex[] = "0123456789abcdef";
				std::string session_id(bytes.size() * 2, '0');
				for (size_t i = 0; i < bytes.size(); ++i)
				{
					session_id[i * 2] = hex[bytes[i] >> 4];
					session_id[i * 2 + 1] = hex[bytes[i] & 0x0f];
				}
				return session_id;
			}

			std::string BuildKey(const std::string& session_id) const
			{
				return std::string(kAdminSessionPrefix) + session_id;
			}

			bool ParseCookie(const std::string& cookie_header, std::string* session_id) const
			{
				if (session_id == nullptr || cookie_header.empty())
				{
					return false;
				}

				size_t start = 0;
				while (start < cookie_header.size())
				{
					size_t end = cookie_header.find(';', start);
					std::string token = end == std::string::npos ? cookie_header.substr(start) : cookie_header.substr(start, end - start);
					size_t left = token.find_first_not_of(' ');
					if (left != std::string::npos)
					{
						size_t right = token.find_last_not_of(' ');
						token = token.substr(left, right - left + 1);
						size_t eq_pos = token.find('=');
						if (eq_pos != std::string::npos)
						{
							std::string name = token.substr(0, eq_pos);
							std::string value = token.substr(eq_pos + 1);
							if (name == kAdminSessionCookieName)
							{
								*session_id = value;
								return true;
							}
						}
					}

					if (end == std::string::npos)
					{
						break;
					}
					start = end + 1;
				}

				return false;
			}

			bool _redis_enabled = false;
			std::string _redis_address;
		};
}
