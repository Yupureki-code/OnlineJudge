#include "../include/oj_admin.hpp"

#include <algorithm>
#include <atomic>
#include <cctype>
#include <chrono>
#include <fstream>
#include <functional>
#include <sstream>

using namespace httplib;
using namespace ns_log;

namespace
{
constexpr const char* kAdminVersion = "v0.2.0";

std::atomic<unsigned long long> g_request_seq{0};

std::string Trim(const std::string& s)
{
	size_t start = 0;
	while (start < s.size() && std::isspace(static_cast<unsigned char>(s[start])))
	{
		++start;
	}

	size_t end = s.size();
	while (end > start && std::isspace(static_cast<unsigned char>(s[end - 1])))
	{
		--end;
	}
	return s.substr(start, end - start);
}

int ParsePositiveIntParam(const Request& req, const std::string& key, int default_value, int min_value, int max_value)
{
	if (!req.has_param(key))
	{
		return default_value;
	}
	try
	{
		int value = std::stoi(req.get_param_value(key));
		if (value < min_value) return min_value;
		if (value > max_value) return max_value;
		return value;
	}
	catch (...)
	{
		return default_value;
	}
}

bool ParseJsonBody(const Request& req, Json::Value* out)
{
	if (out == nullptr || req.body.empty())
	{
		return false;
	}

	Json::CharReaderBuilder builder;
	std::string errs;
	std::istringstream ss(req.body);
	return Json::parseFromStream(builder, ss, out, &errs);
}

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

std::string ReadHtmlFile(const std::string& path)
{
	std::ifstream in(path);
	if (!in.is_open())
	{
		return "";
	}
	std::string html((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
	in.close();
	return html;
}

std::string InjectUserInfo(const std::string& html, ns_control::User* user, bool isLoggedIn)
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

bool IsValidAdminRole(const std::string& role)
{
	return role == "super_admin" || role == "admin";
}

void ReplyBadRequest(Response& rep, const std::string& message)
{
	Json::Value response;
	response["success"] = false;
	response["error_code"] = "BAD_REQUEST";
	response["message"] = message;
	Json::FastWriter writer;
	rep.status = 400;
	rep.set_content(writer.write(response), "application/json;charset=utf-8");
}

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

bool RequireAdmin(ns_model::Model* model,
				  const httplib::Request& req,
				  ns_control::User* user,
				  ns_model::AdminAccount* admin,
				  const std::function<bool(const Request&, ns_control::User*)>& getCurrentUser,
				  const std::function<void(Response&)>& addCORSHeaders,
				  httplib::Response& rep,
				  bool super_only = false)
{
	if (user == nullptr || admin == nullptr || model == nullptr)
	{
		return false;
	}

	if (!getCurrentUser(req, user))
	{
		Json::Value response;
		response["success"] = false;
		response["error_code"] = "UNAUTHORIZED";
		Json::FastWriter writer;
		addCORSHeaders(rep);
		rep.status = 401;
		rep.set_content(writer.write(response), "application/json;charset=utf-8");
		return false;
	}

	if (!model->GetAdminByUid(user->uid, admin))
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

void TryWriteAudit(ns_model::Model* model,
			 const std::string& request_id,
			 const ns_model::AdminAccount& operator_admin,
			 const std::string& action,
			 const std::string& resource_type,
			 const std::string& result,
			 const Json::Value& payload)
{
	if (model == nullptr)
	{
		return;
	}

	Json::FastWriter writer;
	ns_model::AdminAuditLog log;
	log.request_id = request_id;
	log.operator_admin_id = operator_admin.admin_id;
	log.operator_uid = operator_admin.uid;
	log.operator_role = operator_admin.role;
	log.action = action;
	log.resource_type = resource_type;
	log.result = result;
	log.payload_text = writer.write(payload);

	if (!model->InsertAdminAuditLog(log))
	{
		logger(WARNING) << "failed to write admin audit log, request_id=" << request_id;
	}
}

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
}

namespace ns_admin
{
AdminServer::AdminServer()
{}

void AdminServer::RegisterRoutes(httplib::Server& svr)
{
	auto addCORSHeaders = [](Response& rep) {
		rep.set_header("Access-Control-Allow-Origin", "*");
		rep.set_header("Access-Control-Allow-Methods", "POST, GET, DELETE, OPTIONS");
		rep.set_header("Access-Control-Allow-Headers", "Content-Type");
	};

	auto getCurrentUser = [this](const Request& req, ns_control::User* user) -> bool {
		std::string cookie_header = req.get_header_value("Cookie");
		return _ctl.GetSessionUser(cookie_header, user);
	};

	auto model = _ctl.GetModel();

	svr.Get("/healthz", [](const Request& req, Response& rep) {
		(void)req;
		rep.set_content("ok", "text/plain;charset=utf-8");
	});

	svr.Get("/api/admin/version", [&addCORSHeaders](const Request& req, Response& rep) {
		(void)req;
		Json::Value response;
		response["success"] = true;
		response["service"] = "oj_admin";
		response["version"] = kAdminVersion;
		response["build_date"] = __DATE__;
		response["build_time"] = __TIME__;

		Json::FastWriter writer;
		addCORSHeaders(rep);
		rep.set_content(writer.write(response), "application/json;charset=utf-8");
	});

	svr.set_mount_point("/admin", HTML_PATH + std::string("/admin"));

	svr.Get("/admin", [this, &getCurrentUser](const Request& req, Response& rep){
		std::string html;
		_ctl.GetStaticHtml("admin/index.html", &html);

		ns_control::User user;
		bool isLoggedIn = getCurrentUser(req, &user);

		html = InjectUserInfo(html, &user, isLoggedIn);
		rep.set_content(html, "text/html;charset=utf-8");
	});

	svr.Get("/api/admin/overview", [this, model, &getCurrentUser, &addCORSHeaders](const Request& req, Response& rep) {
		std::string request_id = BuildRequestId(req);
		Json::Value response;
		ns_control::User current_user;
		ns_model::AdminAccount current_admin;
		if (!RequireAdmin(model, req, &current_user, &current_admin, getCurrentUser, addCORSHeaders, rep, false))
		{
			return;
		}

		int question_count = 0;
		int user_count = 0;
		int admin_count = 0;
		bool ok = BuildAdminOverview(model, &question_count, &user_count, &admin_count);
		response["success"] = ok;
		if (!ok)
		{
			response["error_code"] = "LOAD_OVERVIEW_FAILED";
			rep.status = 500;
		}
		else
		{
			auto m = model->GetCacheMetricsSnapshot();
			response["data"]["question_count"] = question_count;
			response["data"]["user_count"] = user_count;
			response["data"]["admin_count"] = admin_count;
			response["data"]["cache"]["list_requests"] = Json::Int64(m.list_requests);
			response["data"]["cache"]["list_hits"] = Json::Int64(m.list_hits);
			response["data"]["cache"]["detail_requests"] = Json::Int64(m.detail_requests);
			response["data"]["cache"]["detail_hits"] = Json::Int64(m.detail_hits);
		}

		Json::Value extra;
		extra["request_id"] = request_id;
		TryWriteAudit(model, request_id, current_admin, "admin.overview", "system", ok ? "success" : "failed", BuildPayload(req, rep.status == 0 ? 200 : rep.status, extra));

		Json::FastWriter writer;
		addCORSHeaders(rep);
		rep.set_content(writer.write(response), "application/json;charset=utf-8");
	});

	svr.Get("/api/admin/users", [this, model, &getCurrentUser, &addCORSHeaders](const Request& req, Response& rep) {
		std::string request_id = BuildRequestId(req);
		Json::Value response;
		ns_control::User current_user;
		ns_model::AdminAccount current_admin;
		if (!RequireAdmin(model, req, &current_user, &current_admin, getCurrentUser, addCORSHeaders, rep, false))
		{
			return;
		}

		int page = ParsePositiveIntParam(req, "page", 1, 1, 1000000);
		int size = ParsePositiveIntParam(req, "size", 20, 1, 200);
		std::string keyword = req.has_param("q") ? Trim(req.get_param_value("q")) : "";

		std::vector<ns_model::UserListItem> users;
		int total_count = 0;
		bool ok = model->GetUsersPaged(page, size, keyword, &users, &total_count);

		response["success"] = ok;
		if (!ok)
		{
			response["error_code"] = "LOAD_USERS_FAILED";
			rep.status = 500;
		}
		else
		{
			response["data"]["page"] = page;
			response["data"]["size"] = size;
			response["data"]["total_count"] = total_count;
			Json::Value rows(Json::arrayValue);
			for (const auto& u : users)
			{
				Json::Value item;
				item["uid"] = u.uid;
				item["name"] = u.name;
				item["email"] = u.email;
				item["create_time"] = u.create_time;
				item["last_login"] = u.last_login;
				rows.append(item);
			}
			response["data"]["rows"] = rows;
		}

		Json::Value extra;
		extra["request_id"] = request_id;
		extra["page"] = page;
		extra["size"] = size;
		TryWriteAudit(model, request_id, current_admin, "user.list", "user", ok ? "success" : "failed", BuildPayload(req, rep.status == 0 ? 200 : rep.status, extra));

		Json::FastWriter writer;
		addCORSHeaders(rep);
		rep.set_content(writer.write(response), "application/json;charset=utf-8");
	});

	svr.Get("/api/admin/questions", [this, model, &getCurrentUser, &addCORSHeaders](const Request& req, Response& rep) {
		std::string request_id = BuildRequestId(req);
		Json::Value response;
		ns_control::User current_user;
		ns_model::AdminAccount current_admin;
		if (!RequireAdmin(model, req, &current_user, &current_admin, getCurrentUser, addCORSHeaders, rep, false))
		{
			return;
		}

		int page = ParsePositiveIntParam(req, "page", 1, 1, 1000000);
		int size = ParsePositiveIntParam(req, "size", 20, 1, 200);
		std::string keyword = req.has_param("q") ? Trim(req.get_param_value("q")) : "";

		std::vector<ns_model::Question> all;
		bool ok = model->GetAllQuestions(&all);
		response["success"] = ok;
		if (!ok)
		{
			response["error_code"] = "LOAD_QUESTIONS_FAILED";
			rep.status = 500;
		}
		else
		{
			if (!keyword.empty())
			{
				std::vector<ns_model::Question> filtered;
				for (const auto& q : all)
				{
					if (q.number.find(keyword) != std::string::npos || q.title.find(keyword) != std::string::npos)
					{
						filtered.push_back(q);
					}
				}
				all.swap(filtered);
			}

			std::sort(all.begin(), all.end(), [](const ns_model::Question& a, const ns_model::Question& b){
				return std::atoi(a.number.c_str()) < std::atoi(b.number.c_str());
			});

			int total_count = static_cast<int>(all.size());
			int offset = (page - 1) * size;
			Json::Value rows(Json::arrayValue);
			for (int i = offset; i < std::min(offset + size, total_count); ++i)
			{
				const auto& q = all[i];
				Json::Value item;
				item["number"] = q.number;
				item["title"] = q.title;
				item["star"] = q.star;
				item["cpu_limit"] = q.cpu_limit;
				item["memory_limit"] = q.memory_limit;
				rows.append(item);
			}

			response["data"]["page"] = page;
			response["data"]["size"] = size;
			response["data"]["total_count"] = total_count;
			response["data"]["rows"] = rows;
		}

		Json::Value extra;
		extra["request_id"] = request_id;
		extra["page"] = page;
		extra["size"] = size;
		TryWriteAudit(model, request_id, current_admin, "question.list", "question", ok ? "success" : "failed", BuildPayload(req, rep.status == 0 ? 200 : rep.status, extra));

		Json::FastWriter writer;
		addCORSHeaders(rep);
		rep.set_content(writer.write(response), "application/json;charset=utf-8");
	});

	svr.Get("/api/admin/accounts", [this, model, &getCurrentUser, &addCORSHeaders](const Request& req, Response& rep) {
		std::string request_id = BuildRequestId(req);
		Json::Value response;
		ns_control::User current_user;
		ns_model::AdminAccount current_admin;
		if (!RequireAdmin(model, req, &current_user, &current_admin, getCurrentUser, addCORSHeaders, rep, true))
		{
			return;
		}

		int page = ParsePositiveIntParam(req, "page", 1, 1, 1000000);
		int size = ParsePositiveIntParam(req, "size", 20, 1, 200);
		std::string keyword = req.has_param("q") ? Trim(req.get_param_value("q")) : "";

		std::vector<ns_model::AdminAccount> admins;
		int total_count = 0;
		bool ok = model->ListAdminsPaged(page, size, keyword, &admins, &total_count);
		response["success"] = ok;
		if (!ok)
		{
			response["error_code"] = "LOAD_ADMINS_FAILED";
			rep.status = 500;
		}
		else
		{
			response["data"]["page"] = page;
			response["data"]["size"] = size;
			response["data"]["total_count"] = total_count;
			Json::Value rows(Json::arrayValue);
			for (const auto& a : admins)
			{
				Json::Value item;
				item["admin_id"] = a.admin_id;
				item["uid"] = a.uid;
				item["role"] = a.role;
				item["created_at"] = a.created_at;
				rows.append(item);
			}
			response["data"]["rows"] = rows;
		}

		Json::Value extra;
		extra["request_id"] = request_id;
		extra["page"] = page;
		extra["size"] = size;
		TryWriteAudit(model, request_id, current_admin, "admin.account.list", "admin_account", ok ? "success" : "failed", BuildPayload(req, rep.status == 0 ? 200 : rep.status, extra));

		Json::FastWriter writer;
		addCORSHeaders(rep);
		rep.set_content(writer.write(response), "application/json;charset=utf-8");
	});

	svr.Get(R"(/api/admin/accounts/(\d+))", [this, model, &getCurrentUser, &addCORSHeaders](const Request& req, Response& rep) {
		std::string request_id = BuildRequestId(req);
		Json::Value response;
		ns_control::User current_user;
		ns_model::AdminAccount current_admin;
		if (!RequireAdmin(model, req, &current_user, &current_admin, getCurrentUser, addCORSHeaders, rep, true))
		{
			return;
		}

		int admin_id = std::atoi(std::string(req.matches[1]).c_str());
		ns_model::AdminAccount target;
		bool ok = model->GetAdminByAdminId(admin_id, &target);
		response["success"] = ok;
		if (!ok)
		{
			response["error_code"] = "ADMIN_NOT_FOUND";
			rep.status = 404;
		}
		else
		{
			response["data"]["admin_id"] = target.admin_id;
			response["data"]["uid"] = target.uid;
			response["data"]["role"] = target.role;
			response["data"]["created_at"] = target.created_at;
		}

		Json::Value extra;
		extra["request_id"] = request_id;
		extra["admin_id"] = admin_id;
		TryWriteAudit(model, request_id, current_admin, "admin.account.get", "admin_account", ok ? "success" : "failed", BuildPayload(req, rep.status == 0 ? 200 : rep.status, extra));

		Json::FastWriter writer;
		addCORSHeaders(rep);
		rep.set_content(writer.write(response), "application/json;charset=utf-8");
	});

	svr.Post("/api/admin/accounts", [this, model, &getCurrentUser, &addCORSHeaders](const Request& req, Response& rep) {
		std::string request_id = BuildRequestId(req);
		Json::Value response;
		ns_control::User current_user;
		ns_model::AdminAccount current_admin;
		if (!RequireAdmin(model, req, &current_user, &current_admin, getCurrentUser, addCORSHeaders, rep, true))
		{
			return;
		}

		Json::Value in;
		if (!ParseJsonBody(req, &in))
		{
			addCORSHeaders(rep);
			ReplyBadRequest(rep, "请求体必须是 JSON");
			return;
		}

		if (!in.isMember("uid") || !in["uid"].isInt())
		{
			addCORSHeaders(rep);
			ReplyBadRequest(rep, "uid 必须是整数");
			return;
		}
		int uid = in["uid"].asInt();
		std::string role = in.isMember("role") && in["role"].isString() ? Trim(in["role"].asString()) : "admin";
		std::string password_hash = in.isMember("password_hash") && in["password_hash"].isString() ? in["password_hash"].asString() : "";

		if (!IsValidAdminRole(role))
		{
			response["success"] = false;
			response["error_code"] = "INVALID_ROLE";
			rep.status = 400;
			Json::FastWriter writer;
			addCORSHeaders(rep);
			rep.set_content(writer.write(response), "application/json;charset=utf-8");
			return;
		}

		ns_control::User target_user;
		if (!this->_ctl.GetModel()->GetUserById(uid, &target_user))
		{
			response["success"] = false;
			response["error_code"] = "USER_NOT_FOUND";
			rep.status = 404;
			Json::FastWriter writer;
			addCORSHeaders(rep);
			rep.set_content(writer.write(response), "application/json;charset=utf-8");
			return;
		}

		ns_model::AdminAccount existing;
		if (model->GetAdminByUid(uid, &existing))
		{
			response["success"] = false;
			response["error_code"] = "ADMIN_ALREADY_EXISTS";
			rep.status = 409;
			Json::FastWriter writer;
			addCORSHeaders(rep);
			rep.set_content(writer.write(response), "application/json;charset=utf-8");
			return;
		}

		if (role == "super_admin")
		{
			int super_count = 0;
			if (!model->GetRoleCount("super_admin", &super_count))
			{
				response["success"] = false;
				response["error_code"] = "COUNT_SUPER_ADMIN_FAILED";
				rep.status = 500;
				Json::FastWriter writer;
				addCORSHeaders(rep);
				rep.set_content(writer.write(response), "application/json;charset=utf-8");
				return;
			}
			if (super_count > 0)
			{
				response["success"] = false;
				response["error_code"] = "ONLY_ONE_SUPER_ADMIN";
				rep.status = 409;
				Json::FastWriter writer;
				addCORSHeaders(rep);
				rep.set_content(writer.write(response), "application/json;charset=utf-8");
				return;
			}
		}

		bool ok = model->CreateAdminAccount(uid, password_hash, role);
		response["success"] = ok;
		if (!ok)
		{
			response["error_code"] = "CREATE_ADMIN_FAILED";
			rep.status = 500;
		}
		else
		{
			response["data"]["uid"] = uid;
			response["data"]["role"] = role;
		}

		Json::Value extra;
		extra["request_id"] = request_id;
		extra["uid"] = uid;
		extra["role"] = role;
		TryWriteAudit(model, request_id, current_admin, "admin.account.create", "admin_account", ok ? "success" : "failed", BuildPayload(req, rep.status == 0 ? 200 : rep.status, extra));

		Json::FastWriter writer;
		addCORSHeaders(rep);
		rep.set_content(writer.write(response), "application/json;charset=utf-8");
	});

	svr.Put(R"(/api/admin/accounts/(\d+))", [this, model, &getCurrentUser, &addCORSHeaders](const Request& req, Response& rep) {
		std::string request_id = BuildRequestId(req);
		Json::Value response;
		ns_control::User current_user;
		ns_model::AdminAccount current_admin;
		if (!RequireAdmin(model, req, &current_user, &current_admin, getCurrentUser, addCORSHeaders, rep, true))
		{
			return;
		}

		int target_admin_id = std::atoi(std::string(req.matches[1]).c_str());
		ns_model::AdminAccount target;
		if (!model->GetAdminByAdminId(target_admin_id, &target))
		{
			response["success"] = false;
			response["error_code"] = "ADMIN_NOT_FOUND";
			rep.status = 404;
			Json::FastWriter writer;
			addCORSHeaders(rep);
			rep.set_content(writer.write(response), "application/json;charset=utf-8");
			return;
		}

		Json::Value in;
		if (!ParseJsonBody(req, &in))
		{
			addCORSHeaders(rep);
			ReplyBadRequest(rep, "请求体必须是 JSON");
			return;
		}

		if (!in.isMember("role") || !in["role"].isString())
		{
			addCORSHeaders(rep);
			ReplyBadRequest(rep, "role 不能为空");
			return;
		}
		std::string new_role = Trim(in["role"].asString());
		if (!IsValidAdminRole(new_role))
		{
			response["success"] = false;
			response["error_code"] = "INVALID_ROLE";
			rep.status = 400;
			Json::FastWriter writer;
			addCORSHeaders(rep);
			rep.set_content(writer.write(response), "application/json;charset=utf-8");
			return;
		}

		if (current_admin.admin_id == target.admin_id && target.role == "super_admin" && new_role != "super_admin")
		{
			response["success"] = false;
			response["error_code"] = "SUPER_ADMIN_SELF_DOWNGRADE_FORBIDDEN";
			rep.status = 400;
			Json::FastWriter writer;
			addCORSHeaders(rep);
			rep.set_content(writer.write(response), "application/json;charset=utf-8");
			return;
		}

		if (target.role == "super_admin" && new_role != "super_admin")
		{
			int super_count = 0;
			if (!model->GetRoleCount("super_admin", &super_count) || super_count <= 1)
			{
				response["success"] = false;
				response["error_code"] = "LAST_SUPER_ADMIN_PROTECTED";
				rep.status = 409;
				Json::FastWriter writer;
				addCORSHeaders(rep);
				rep.set_content(writer.write(response), "application/json;charset=utf-8");
				return;
			}
		}

		if (target.role != "super_admin" && new_role == "super_admin")
		{
			int super_count = 0;
			if (!model->GetRoleCount("super_admin", &super_count))
			{
				response["success"] = false;
				response["error_code"] = "COUNT_SUPER_ADMIN_FAILED";
				rep.status = 500;
				Json::FastWriter writer;
				addCORSHeaders(rep);
				rep.set_content(writer.write(response), "application/json;charset=utf-8");
				return;
			}
			if (super_count > 0)
			{
				response["success"] = false;
				response["error_code"] = "ONLY_ONE_SUPER_ADMIN";
				rep.status = 409;
				Json::FastWriter writer;
				addCORSHeaders(rep);
				rep.set_content(writer.write(response), "application/json;charset=utf-8");
				return;
			}
		}

		bool ok = model->UpdateAdminRole(target_admin_id, new_role);
		response["success"] = ok;
		if (!ok)
		{
			response["error_code"] = "UPDATE_ADMIN_FAILED";
			rep.status = 500;
		}
		else
		{
			response["data"]["admin_id"] = target_admin_id;
			response["data"]["role"] = new_role;
		}

		Json::Value extra;
		extra["request_id"] = request_id;
		extra["admin_id"] = target_admin_id;
		extra["old_role"] = target.role;
		extra["new_role"] = new_role;
		TryWriteAudit(model, request_id, current_admin, "admin.account.update", "admin_account", ok ? "success" : "failed", BuildPayload(req, rep.status == 0 ? 200 : rep.status, extra));

		Json::FastWriter writer;
		addCORSHeaders(rep);
		rep.set_content(writer.write(response), "application/json;charset=utf-8");
	});

	svr.Delete(R"(/api/admin/accounts/(\d+))", [this, model, &getCurrentUser, &addCORSHeaders](const Request& req, Response& rep) {
		std::string request_id = BuildRequestId(req);
		Json::Value response;
		ns_control::User current_user;
		ns_model::AdminAccount current_admin;
		if (!RequireAdmin(model, req, &current_user, &current_admin, getCurrentUser, addCORSHeaders, rep, true))
		{
			return;
		}

		int target_admin_id = std::atoi(std::string(req.matches[1]).c_str());
		ns_model::AdminAccount target;
		if (!model->GetAdminByAdminId(target_admin_id, &target))
		{
			response["success"] = false;
			response["error_code"] = "ADMIN_NOT_FOUND";
			rep.status = 404;
			Json::FastWriter writer;
			addCORSHeaders(rep);
			rep.set_content(writer.write(response), "application/json;charset=utf-8");
			return;
		}

		if (current_admin.admin_id == target.admin_id)
		{
			response["success"] = false;
			response["error_code"] = "ADMIN_SELF_DELETE_FORBIDDEN";
			rep.status = 400;
			Json::FastWriter writer;
			addCORSHeaders(rep);
			rep.set_content(writer.write(response), "application/json;charset=utf-8");
			return;
		}

		if (target.role == "super_admin")
		{
			int super_count = 0;
			if (!model->GetRoleCount("super_admin", &super_count) || super_count <= 1)
			{
				response["success"] = false;
				response["error_code"] = "LAST_SUPER_ADMIN_PROTECTED";
				rep.status = 409;
				Json::FastWriter writer;
				addCORSHeaders(rep);
				rep.set_content(writer.write(response), "application/json;charset=utf-8");
				return;
			}
		}

		bool ok = model->DeleteAdminAccount(target_admin_id);
		response["success"] = ok;
		if (!ok)
		{
			response["error_code"] = "DELETE_ADMIN_FAILED";
			rep.status = 500;
		}

		Json::Value extra;
		extra["request_id"] = request_id;
		extra["admin_id"] = target_admin_id;
		extra["target_role"] = target.role;
		TryWriteAudit(model, request_id, current_admin, "admin.account.delete", "admin_account", ok ? "success" : "failed", BuildPayload(req, rep.status == 0 ? 200 : rep.status, extra));

		Json::FastWriter writer;
		addCORSHeaders(rep);
		rep.set_content(writer.write(response), "application/json;charset=utf-8");
	});

	svr.Get("/api/admin/audit/logs", [this, model, &getCurrentUser, &addCORSHeaders](const Request& req, Response& rep) {
		std::string request_id = BuildRequestId(req);
		Json::Value response;
		ns_control::User current_user;
		ns_model::AdminAccount current_admin;
		if (!RequireAdmin(model, req, &current_user, &current_admin, getCurrentUser, addCORSHeaders, rep, false))
		{
			return;
		}

		int page = ParsePositiveIntParam(req, "page", 1, 1, 1000000);
		int size = ParsePositiveIntParam(req, "size", 20, 1, 200);
		int operator_uid = ParsePositiveIntParam(req, "operator_uid", 0, 0, 2147483647);
		std::string action = req.has_param("action") ? Trim(req.get_param_value("action")) : "";
		std::string result = req.has_param("result") ? Trim(req.get_param_value("result")) : "";

		std::vector<ns_model::AdminAuditLog> logs;
		int total_count = 0;
		bool ok = model->ListAdminAuditLogsPaged(page, size, action, operator_uid, result, &logs, &total_count);
		response["success"] = ok;
		if (!ok)
		{
			response["error_code"] = "LOAD_AUDIT_LOGS_FAILED";
			rep.status = 500;
		}
		else
		{
			response["data"]["page"] = page;
			response["data"]["size"] = size;
			response["data"]["total_count"] = total_count;
			Json::Value rows(Json::arrayValue);
			for (const auto& item : logs)
			{
				Json::Value row;
				row["request_id"] = item.request_id;
				row["operator_admin_id"] = item.operator_admin_id;
				row["operator_uid"] = item.operator_uid;
				row["operator_role"] = item.operator_role;
				row["action"] = item.action;
				row["resource_type"] = item.resource_type;
				row["result"] = item.result;
				row["payload_text"] = item.payload_text;
				rows.append(row);
			}
			response["data"]["rows"] = rows;
		}

		Json::Value extra;
		extra["request_id"] = request_id;
		extra["page"] = page;
		extra["size"] = size;
		TryWriteAudit(model, request_id, current_admin, "admin.audit.list", "admin_audit", ok ? "success" : "failed", BuildPayload(req, rep.status == 0 ? 200 : rep.status, extra));

		Json::FastWriter writer;
		addCORSHeaders(rep);
		rep.set_content(writer.write(response), "application/json;charset=utf-8");
	});

	svr.Post("/api/auth/logout", [this, &addCORSHeaders](const Request& req, Response& rep) {
		(void)this;
		std::string cookie_header = req.get_header_value("Cookie");
		_ctl.DestroySessionByCookieHeader(cookie_header);
		rep.set_header("Set-Cookie", _ctl.GetClearCookieHeader());

		Json::Value response;
		response["success"] = true;
		Json::FastWriter writer;
		addCORSHeaders(rep);
		rep.set_content(writer.write(response), "application/json;charset=utf-8");
	});

	svr.Options("/api/admin/overview", [&addCORSHeaders](const Request& req, Response& rep) {
		(void)req;
		addCORSHeaders(rep);
		rep.status = 200;
	});

	svr.Options("/api/admin/users", [&addCORSHeaders](const Request& req, Response& rep) {
		(void)req;
		addCORSHeaders(rep);
		rep.status = 200;
	});

	svr.Options("/api/admin/questions", [&addCORSHeaders](const Request& req, Response& rep) {
		(void)req;
		addCORSHeaders(rep);
		rep.status = 200;
	});

	svr.Options("/api/admin/version", [&addCORSHeaders](const Request& req, Response& rep) {
		(void)req;
		addCORSHeaders(rep);
		rep.status = 200;
	});

	svr.Options("/api/admin/accounts", [&addCORSHeaders](const Request& req, Response& rep) {
		(void)req;
		addCORSHeaders(rep);
		rep.status = 200;
	});

	svr.Options(R"(/api/admin/accounts/(\d+))", [&addCORSHeaders](const Request& req, Response& rep) {
		(void)req;
		addCORSHeaders(rep);
		rep.status = 200;
	});

	svr.Options("/api/admin/audit/logs", [&addCORSHeaders](const Request& req, Response& rep) {
		(void)req;
		addCORSHeaders(rep);
		rep.status = 200;
	});

	svr.Options("/api/auth/logout", [&addCORSHeaders](const Request& req, Response& rep) {
		(void)req;
		addCORSHeaders(rep);
		rep.status = 200;
	});

	svr.Options("/api/*", [&addCORSHeaders](const Request& req, Response& rep) {
		(void)req;
		addCORSHeaders(rep);
		rep.status = 200;
	});
}

bool AdminServer::Start(const std::string& host, int port)
{
	Server svr;
	RegisterRoutes(svr);
	logger(INFO) << "admin server start at " << host << ":" << port;
	return svr.listen(host.c_str(), port);
}
}

int main()
{
	ns_admin::AdminServer server;
	server.Start(admin_host, admin_port);
	return 0;
}