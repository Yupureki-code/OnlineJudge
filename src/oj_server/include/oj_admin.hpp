#pragma once

#include <string>
#include <deque>
#include <thread>
#include <condition_variable>
#include <fstream>
#include <cstdio>
#include <httplib.h>
#include "oj_control.hpp"
#include "../../comm/comm.hpp"
#include "../../comm/log_event.hpp"


namespace ns_admin
{
	using namespace ns_util;
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
	public:
		AdminSessionStore()
			: _redis_enabled(!redis_addr.empty())
		{}

		std::string CreateSession(const AdminAccount& admin, const User& user)
		{
			AdminSession session;
			session.session_id = GenerateSessionId();
			session.admin_id = admin.admin_id;
			session.uid = admin.uid;
			session.name = user.name;
			session.email = user.email;
			session.role = admin.role;
			session.create_time = time(nullptr);
			session.last_access_time = session.create_time;
			session.expires_at = session.create_time + kAdminSessionExpireSeconds;

			StoreInMemory(session);
			StoreInRedis(session);
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
			if (LoadFromRedis(session_id, &loaded))
			{
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

		void DestroySessionByCookie(const std::string& cookie_header)
		{
			std::string session_id;
			if (!ParseCookie(cookie_header, &session_id))
			{
				return;
			}

			RemoveFromMemory(session_id);
			DestroyInRedis(session_id);
		}

		std::string BuildSetCookieHeader(const std::string& session_id) const
		{
			time_t expire = time(nullptr) + kAdminSessionExpireSeconds;
			char expire_str[64];
			struct tm tm_info;
			gmtime_r(&expire, &tm_info);
			strftime(expire_str, sizeof(expire_str), "%a, %d %b %Y %H:%M:%S GMT", &tm_info);
			return std::string(kAdminSessionCookieName) + "=" + session_id +
				"; Expires=" + expire_str +
				"; Path=/; HttpOnly; SameSite=Lax";
		}

		std::string BuildClearCookieHeader() const
		{
			return std::string(kAdminSessionCookieName) + "=; Expires=Thu, 01 Jan 1970 00:00:00 GMT; Path=/";
		}

		private:
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

			void StoreInRedis(const AdminSession& session)
			{
				if (!_redis_enabled)
				{
					return;
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
					sw::redis::Redis redis(redis_addr);
					redis.hmset(key, fields.begin(), fields.end());
					redis.expire(key, kAdminSessionExpireSeconds);
				}
				catch (const std::exception& ex)
				{
					logger(WARNING) << "admin redis create session failed: " << ex.what();
				}
				catch (...)
				{
					logger(WARNING) << "admin redis create session failed: unknown exception";
				}
			}

			bool LoadFromRedis(const std::string& session_id, AdminSession* session)
			{
				_redis_enabled = true;
				if (!_redis_enabled || session == nullptr)
				{
					return false;
				}
				std::unordered_map<std::string, std::string> fields;
				std::string key = BuildKey(session_id);

				try
				{
					sw::redis::Redis redis(redis_addr);
					redis.hgetall(key, std::inserter(fields, fields.begin()));
				}
				catch (const std::exception& ex)
				{
					logger(WARNING) << "admin redis hgetall failed: " << ex.what();
					return false;
				}
				catch (...)
				{
					logger(WARNING) << "admin redis hgetall failed: unknown exception";
					return false;
				}
				if (fields.empty())
				{
					return false;
				}

				auto admin_id_it = fields.find("admin_id");
				auto uid_it = fields.find("uid");
				auto name_it = fields.find("name");
				auto email_it = fields.find("email");
				auto role_it = fields.find("role");
				auto create_time_it = fields.find("create_time");
				if (admin_id_it == fields.end() || uid_it == fields.end() || name_it == fields.end() || email_it == fields.end() || role_it == fields.end() || create_time_it == fields.end())
				{
					return false;
				}
				time_t now = time(nullptr);
				session->session_id = session_id;
				try
				{
					session->admin_id = std::stoi(admin_id_it->second);
					session->uid = std::stoi(uid_it->second);
					session->name = name_it->second;
					session->email = email_it->second;
					session->role = role_it->second;
					session->create_time = static_cast<time_t>(std::stoll(create_time_it->second));
				}
				catch (const std::exception& ex)
				{
					logger(WARNING) << "admin redis session decode failed: " << ex.what();
					return false;
				}
				catch (...)
				{
					logger(WARNING) << "admin redis session decode failed: unknown exception";
					return false;
				}
				session->last_access_time = now;
				session->expires_at = now + kAdminSessionExpireSeconds;

				try
				{
					sw::redis::Redis redis(redis_addr);
					redis.hset(key, "last_access_time", std::to_string(static_cast<long long>(session->last_access_time)));
					redis.expire(key, kAdminSessionExpireSeconds);
				}
				catch (const std::exception& ex)
				{
					logger(WARNING) << "admin redis touch session failed: " << ex.what();
					return false;
				}
				catch (...)
				{
					logger(WARNING) << "admin redis touch session failed: unknown exception";
					return false;
				}
				return true;
			}

			void DestroyInRedis(const std::string& session_id)
			{
				if (!_redis_enabled)
				{
					return;
				}
				try
				{
					sw::redis::Redis redis(redis_addr);
					redis.del(BuildKey(session_id));
				}
				catch (const std::exception& ex)
				{
					logger(WARNING) << "admin redis destroy session failed: " << ex.what();
				}
				catch (...)
				{
					logger(WARNING) << "admin redis destroy session failed: unknown exception";
				}
			}

			std::string GenerateSessionId() const
			{
				static thread_local std::mt19937 gen(std::random_device{}());
				std::uniform_int_distribution<> dis(0, 15);
				const char* hex = "0123456789abcdef";
				std::string session_id;
				session_id.reserve(32);
				for (int i = 0; i < 32; ++i)
				{
					session_id.push_back(hex[dis(gen)]);
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
		};
	class AdminServer
	{
	private:
		struct PendingAudit
		{
			std::string request_id;
			AdminAccount operator_admin;
			std::string action;
			std::string resource_type;
			std::string result;
			std::string payload_text;
			int retry_count = 0;
		};

		std::string SerializePendingAudit(const PendingAudit& item)
		{
			Json::Value root(Json::objectValue);
			root["request_id"] = item.request_id;
			root["admin_id"] = item.operator_admin.admin_id;
			root["uid"] = item.operator_admin.uid;
			root["role"] = item.operator_admin.role;
			root["action"] = item.action;
			root["resource_type"] = item.resource_type;
			root["result"] = item.result;
			root["payload_text"] = item.payload_text;
			root["retry_count"] = item.retry_count;
			Json::FastWriter writer;
			return writer.write(root);
		}

		bool DeserializePendingAudit(const std::string& line, PendingAudit* out)
		{
			if (out == nullptr || line.empty())
			{
				return false;
			}
			Json::CharReaderBuilder builder;
			Json::Value root;
			std::string errs;
			std::istringstream ss(line);
			if (!Json::parseFromStream(builder, ss, &root, &errs) || !root.isObject())
			{
				return false;
			}

			out->request_id = root.isMember("request_id") ? root["request_id"].asString() : "";
			out->operator_admin.admin_id = root.isMember("admin_id") ? root["admin_id"].asInt() : 0;
			out->operator_admin.uid = root.isMember("uid") ? root["uid"].asInt() : 0;
			out->operator_admin.role = root.isMember("role") ? root["role"].asString() : "";
			out->action = root.isMember("action") ? root["action"].asString() : "";
			out->resource_type = root.isMember("resource_type") ? root["resource_type"].asString() : "";
			out->result = root.isMember("result") ? root["result"].asString() : "";
			out->payload_text = root.isMember("payload_text") ? root["payload_text"].asString() : "";
			out->retry_count = root.isMember("retry_count") ? root["retry_count"].asInt() : 0;
			return !out->request_id.empty();
		}

		bool AppendAuditToSpool(const PendingAudit& item)
		{
			std::lock_guard<std::mutex> lock(_audit_spool_mtx);
			std::ofstream fout(_audit_spool_path.c_str(), std::ios::app);
			if (!fout.is_open())
			{
				logger(WARNING) << "failed to open audit spool file: " << _audit_spool_path;
				return false;
			}
			fout << SerializePendingAudit(item);
			return true;
		}

		void RecoverAuditFromSpool()
		{
			std::deque<PendingAudit> recovered;
			{
				std::lock_guard<std::mutex> lock(_audit_spool_mtx);
				std::ifstream fin(_audit_spool_path.c_str());
				if (!fin.is_open())
				{
					return;
				}

				std::string line;
				while (std::getline(fin, line))
				{
					PendingAudit item;
					if (DeserializePendingAudit(line, &item))
					{
						recovered.emplace_back(std::move(item));
					}
				}
				fin.close();

				std::ofstream trunc(_audit_spool_path.c_str(), std::ios::trunc);
				(void)trunc;
			}

			for (auto& item : recovered)
			{
				if (!EnqueueAudit(PendingAudit(item)))
				{
					AppendAuditToSpool(item);
				}
			}
			if (!recovered.empty())
			{
				logger(INFO) << "audit spool recovery loaded=" << recovered.size();
			}
		}

		bool SendAuditToOjLog(const PendingAudit& item)
		{
			ns_log_event::LogEvent event;
			event.ts_ms = ns_log_event::NowMs();
			event.service = "oj_admin";
			event.category = ns_log_event::LogCategory::audit;
			event.level = (item.result == "success") ? ns_log::INFO : ns_log::WARNING;
			event.request_id = item.request_id;
			event.action = item.action;
			event.resource_type = item.resource_type;
			event.uid = item.operator_admin.uid;
			event.admin_id = item.operator_admin.admin_id;

			Json::Value payload(Json::objectValue);
			payload["result"] = item.result;
			payload["operator_role"] = item.operator_admin.role;

			if (!item.payload_text.empty())
			{
				Json::CharReaderBuilder builder;
				Json::Value parsed;
				std::string errs;
				std::istringstream ss(item.payload_text);
				if (Json::parseFromStream(builder, ss, &parsed, &errs))
				{
					payload["detail"] = parsed;
				}
				else
				{
					payload["detail_text"] = item.payload_text;
				}
			}
			event.payload = payload;

			Json::FastWriter writer;
			std::string body = writer.write(ns_log_event::ToJson(event));

			httplib::Client cli(_oj_log_host, _oj_log_port);
			cli.set_connection_timeout(0, 100000); // 100ms
			cli.set_read_timeout(0, 100000); // 100ms
			auto res = cli.Post("/api/logs/ingest", body, "application/json;charset=utf-8");
			if (!res)
			{
				return false;
			}
			return res->status == 202 || res->status == 204 || res->status == 200;
		}

		void StartAuditWorker()
		{
			bool expected = false;
			if (!_audit_running.compare_exchange_strong(expected, true, std::memory_order_relaxed))
			{
				return;
			}
			_audit_worker = std::thread([this]() {
				RecoverAuditFromSpool();
				for (;;)
				{
					PendingAudit item;
					{
						std::unique_lock<std::mutex> lock(_audit_mtx);
						_audit_cv.wait(lock, [this]() {
							return !_audit_running.load(std::memory_order_relaxed) || !_audit_queue.empty();
						});

						if (_audit_queue.empty())
						{
							if (!_audit_running.load(std::memory_order_relaxed))
							{
								break;
							}
							continue;
						}

						item = std::move(_audit_queue.front());
						_audit_queue.pop_front();
					}

					if (g_admin_audit_disabled.load(std::memory_order_relaxed))
					{
						continue;
					}

					if (!SendAuditToOjLog(item))
					{
						if (item.retry_count < kAuditRetryMax)
						{
							PendingAudit retry_item = item;
							retry_item.retry_count++;
							if (!EnqueueAudit(std::move(retry_item)))
							{
								logger(WARNING) << "admin audit retry queue full, dropping event, request_id=" << item.request_id;
							}
						}
						else
						{
							logger(WARNING) << "failed to deliver admin audit to oj_log after retries, dropping event, request_id=" << item.request_id;
							AppendAuditToSpool(item);
						}
					}
				}
			});
		}

		void StopAuditWorker()
		{
			_audit_running.store(false, std::memory_order_relaxed);
			_audit_cv.notify_all();
			if (_audit_worker.joinable())
			{
				_audit_worker.join();
			}
		}

		bool EnqueueAudit(PendingAudit&& item)
		{
			{
				std::lock_guard<std::mutex> lock(_audit_mtx);
				if (_audit_queue.size() >= kAuditQueueMax)
				{
					return false;
				}
				_audit_queue.emplace_back(std::move(item));
			}
			_audit_cv.notify_one();
			return true;
		}

		//Json int字段解析
	bool TryParseIntField(const Json::Value& in, const char* key, int* out)
	{
		if (out == nullptr || key == nullptr)
			return false;
		if (!in.isMember(key))
			return false;
		const Json::Value& v = in[key];
		if (v.isInt())
		{
			*out = v.asInt();
			return true;
		}
		if (v.isString())
		{
			*out = std::stoi(StringUtil::Trim(v.asString()));
			return true;
		}
		return false;
	}

	//利用Json构造题目对象
	bool BuildQuestionFromJson(const Json::Value& in,
						Question* out,
						std::string* error_message,
						const std::string* forced_number = nullptr)
	{
		if (out == nullptr || error_message == nullptr)
			return false;
		std::string number;
		if (forced_number != nullptr)
			number = StringUtil::Trim(*forced_number);
		else if (in.isMember("number") && in["number"].isString())
			number = StringUtil::Trim(in["number"].asString());
		if (number.empty() || !std::all_of(number.begin(), number.end(), [](unsigned char ch){ return std::isdigit(ch); }))
		{
			*error_message = "number 必须是纯数字题号";
			return false;
		}
		std::string title = in.isMember("title") && in["title"].isString() ? StringUtil::Trim(in["title"].asString()) : "";
		std::string desc = in.isMember("desc") && in["desc"].isString() ? in["desc"].asString() : "";
		std::string star = in.isMember("star") && in["star"].isString() ? StringUtil::Trim(in["star"].asString()) : "";
		int cpu_limit = 0;
		int memory_limit = 0;

		if (title.empty())
		{
			*error_message = "title 不能为空";
			return false;
		}
		if (star.empty())
		{
			*error_message = "star 不能为空";
			return false;
		}
		if (!TryParseIntField(in, "cpu_limit", &cpu_limit) || cpu_limit <= 0)
		{
			*error_message = "cpu_limit 必须是正整数";
			return false;
		}
		if (!TryParseIntField(in, "memory_limit", &memory_limit) || memory_limit <= 0)
		{
			*error_message = "memory_limit 必须是正整数";
			return false;
		}

		out->number = number;
		out->title = title;
		out->desc = desc;
		out->star = star;
		out->cpu_limit = cpu_limit;
		out->memory_limit = memory_limit;
		return true;
	}
	//请求ID生成
	std::string BuildRequestId(const Request& req)
	{
		std::string request_id = req.get_header_value("X-Request-Id");
		if (!request_id.empty())
		{
			return request_id;
		}

		auto now = std::chrono::system_clock::now().time_since_epoch();
		long long ms = std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
		unsigned long long seq = g_request_seq.fetch_add(1, std::memory_order_relaxed) + 1;
		return "req_" + std::to_string(ms) + "_" + std::to_string(seq);
	}
	//用户信息注入HTML
	std::string InjectUserInfo(const std::string& html,
					User* user,
					const AdminAccount* admin,
					bool isLoggedIn)
	{
		std::string result = html;

		std::string userInfoScript = R"(<script>)";
		userInfoScript += "var SERVER_USER_INFO = {";
		userInfoScript += "isLoggedIn: " + std::string(isLoggedIn ? "true" : "false");
		if (isLoggedIn && user)
		{
			userInfoScript += ", name: \"" + user->name + "\"";
			userInfoScript += ", email: \"" + user->email + "\"";
			userInfoScript += ", uid: " + std::to_string(user->uid);
			if (admin != nullptr)
			{
				userInfoScript += ", admin_id: " + std::to_string(admin->admin_id);
				userInfoScript += ", role: \"" + admin->role + "\"";
			}
		}
		userInfoScript += "};";
		userInfoScript += "</script>";

		size_t bodyEndPos = result.find("</body>");
		if (bodyEndPos != std::string::npos)
		{
			result.insert(bodyEndPos, userInfoScript);
		}
		else
		{
			result += userInfoScript;
		}

		return result;
	}
	//管理员身份校验
	bool IsValidAdminRole(const std::string& role)
	{
		return role == "super_admin" || role == "admin";
	}
	//邮箱合法性校验
	bool IsValidEmail(const std::string& email)
	{
		static const std::regex kEmailPattern(R"(^[A-Za-z0-9._%+\-]+@[A-Za-z0-9.\-]+\.[A-Za-z]{2,}$)");
		return std::regex_match(email, kEmailPattern);
	}
	//验证码校验
	bool IsValidAuthCode(const std::string& code)
	{
		if (code.size() != 6)
		{
			return false;
		}
		return std::all_of(code.begin(), code.end(), [](unsigned char ch){ return std::isdigit(ch); });
	}
	//Json提取邮箱并验证合法性
	bool ExtractAndValidateEmail(const Json::Value& in_value, std::string* email)
	{
		if (email == nullptr || !in_value.isMember("email") || !in_value["email"].isString())
			return false;
		*email = StringUtil::Trim(in_value["email"].asString());
		if (email->empty() || !IsValidEmail(*email))
			return false;
		return true;
	}
	//Json提取密码并验证合法性
	bool ExtractAndValidatePassword(const Json::Value& in_value, std::string* password)
	{
		if (password == nullptr || !in_value.isMember("password") || !in_value["password"].isString())
			return false;
		*password = in_value["password"].asString();
		if (password->empty())
			return false;
		return true;
	}
	//概览数据聚合
	bool BuildAdminOverview(ns_model::Model* model, int* question_count, int* user_count, int* admin_count)
	{
		if (model == nullptr || question_count == nullptr || user_count == nullptr || admin_count == nullptr)
		{
			return false;
		}
		if (!model->GetQuestionCount(question_count))
		{
			return false;
		}
		if (!model->GetUserCount(user_count))
		{
			return false;
		}
		if (!model->GetAdminCount(admin_count))
		{
			return false;
		}
		return true;
	}
	//缓存命中率计算
	double CalcRate(long long hits, long long requests)
	{
		if (requests <= 0)
		{
			return 0.0;
		}
		return static_cast<double>(hits) * 100.0 / static_cast<double>(requests);
	}
	//鉴权中间件
	bool RequireAdmin(const httplib::Request& req,
					User* user,
					AdminAccount* admin,
					const std::function<bool(const Request&, User*, AdminAccount*)>& getCurrentAdmin,
					const std::function<void(Response&)>& addCORSHeaders,
					httplib::Response& rep,
					bool super_only = false)
	{
		if (user == nullptr || admin == nullptr)
		{
			return false;
		}

		if (!getCurrentAdmin(req, user, admin))
		{
			Json::Value response;
			response["success"] = false;
			response["error_code"] = "NOT_FOUND";
			Json::FastWriter writer;
			addCORSHeaders(rep);
			rep.status = 404;
			rep.set_content(writer.write(response), "application/json;charset=utf-8");
			return false;
		}

		if (super_only && admin->role != "super_admin")
		{
			Json::Value response;
			response["success"] = false;
			response["error_code"] = "FORBIDDEN";
			Json::FastWriter writer;
			addCORSHeaders(rep);
			rep.status = 403;
			rep.set_content(writer.write(response), "application/json;charset=utf-8");
			return false;
		}

		return true;
	}
	//审计日志写入
	void TryWriteAudit(ns_model::Model* model,
				const std::string& request_id,
				const AdminAccount& operator_admin,
				const std::string& action,
				const std::string& resource_type,
				const std::string& result,
				const Json::Value& payload)
	{
		if (model == nullptr || g_admin_audit_disabled.load(std::memory_order_relaxed))
		{
			return;
		}
		try
		{
			Json::FastWriter writer;
			PendingAudit item;
			item.request_id = request_id;
			item.operator_admin = operator_admin;
			item.action = action;
			item.resource_type = resource_type;
			item.result = result;
			item.payload_text = writer.write(payload);

			if (!EnqueueAudit(PendingAudit(item)))
			{
				logger(WARNING) << "admin audit queue full, fallback to spool, request_id=" << request_id;
				AppendAuditToSpool(item);
			}
		}
		catch (const std::exception& ex)
		{
			logger(WARNING) << "admin audit enqueue threw exception, request_id="
							<< request_id << " what=" << ex.what();
		}
		catch (...)
		{
			logger(WARNING) << "admin audit enqueue threw unknown exception, request_id=" << request_id;
		}
	}
	//审计载荷构造
	Json::Value BuildPayload(const Request& req, int status_code, const Json::Value& extra)
	{
		Json::Value payload;
		payload["path"] = req.path;
		payload["method"] = req.method;
		payload["ip"] = req.remote_addr;
		payload["user_agent"] = req.get_header_value("User-Agent");
		payload["status_code"] = status_code;
		payload["extra"] = extra;
		return payload;
	}
	public:
		AdminServer();
		~AdminServer();

		void RegisterRoutes(httplib::Server& svr);
		bool Start(const std::string& host = "0.0.0.0", int port = 8090);

	private:
		AdminSessionStore _store;
		ns_control::Control _ctl;
		std::atomic<unsigned long long> g_request_seq{0};
		std::atomic<bool> g_admin_audit_disabled{false};
		static constexpr std::size_t kAuditQueueMax = 10000;
		static constexpr int kAuditRetryMax = 3;
		std::string _oj_log_host = ns_runtime_cfg::GetEnvOrDefault("OJ_LOG_HOST", "127.0.0.1");
		int _oj_log_port = ns_runtime_cfg::GetEnvIntOrDefault("OJ_LOG_PORT", 8100);
		std::string _audit_spool_path = ns_runtime_cfg::GetEnvOrDefault("OJ_ADMIN_AUDIT_SPOOL", LOG_PATH + std::string("oj_admin_audit_spool.jsonl"));
		std::mutex _audit_mtx;
		std::condition_variable _audit_cv;
		std::deque<PendingAudit> _audit_queue;
		std::mutex _audit_spool_mtx;
		std::atomic<bool> _audit_running{false};
		std::thread _audit_worker;	
	};
}

