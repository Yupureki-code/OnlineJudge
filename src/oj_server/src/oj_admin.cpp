#include "../include/oj_admin.hpp"
#include <Logger/logstrategy.h>
#include <algorithm>
#include <cctype>
#include <functional>

using namespace httplib;
using namespace ns_log;
using namespace ns_admin;
using namespace ns_util;

namespace ns_admin
{
AdminServer::AdminServer()
{
	StartAuditWorker();
}

AdminServer::~AdminServer()
{
	StopAuditWorker();
}

void AdminServer::RegisterRoutes(httplib::Server& svr)
{
	auto addCORSHeaders = [](Response& rep) {
		rep.set_header("Access-Control-Allow-Origin", "*");
		rep.set_header("Access-Control-Allow-Methods", "POST, GET, DELETE, OPTIONS");
		rep.set_header("Access-Control-Allow-Headers", "Content-Type");
	};

	auto model = _ctl.GetModel();

	auto getCurrentAdmin = [this](const Request& req, User* user, AdminAccount* admin) -> bool {
		AdminSession session;
		if (!_store.GetSessionByCookie(req.get_header_value("Cookie"), &session))
		{
			return false;
		}
		if (session.admin_id <= 0 || session.uid <= 0)
		{
			return false;
		}

		if (admin != nullptr)
		{
			admin->admin_id = session.admin_id;
			admin->uid = session.uid;
			admin->role = session.role;
		}
		if (user != nullptr)
		{
			user->uid = session.uid;
			user->name = session.name;
			user->email = session.email;
		}
		return true;
	};

	svr.Get("/healthz", [](const Request& req, Response& rep) {
		(void)req;
		rep.set_content("ok", "text/plain;charset=utf-8");
	});

	svr.Get("/api/admin/version", [addCORSHeaders](const Request& req, Response& rep) {
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

	svr.Post("/api/admin/auth/login", [this, model, addCORSHeaders](const Request& req, Response& rep) {
		Json::Value response;
		Json::Value in;
		if (!JsonUtil::ParseJsonBody(req, &in))
		{
			addCORSHeaders(rep);
			HttpUtil::ReplyBadRequest(rep, "请求体必须是 JSON");
			return;
		}

		int admin_id = 0;
		if (!TryParseIntField(in, "admin_id", &admin_id) || admin_id <= 0)
		{
			addCORSHeaders(rep);
			HttpUtil::ReplyBadRequest(rep, "admin_id 必须是正整数");
			return;
		}

		if (!in.isMember("password") || !in["password"].isString())
		{
			addCORSHeaders(rep);
			HttpUtil::ReplyBadRequest(rep, "password 不能为空");
			return;
		}
		std::string password = in["password"].asString();

		AdminAccount admin;
		User user;
		std::string err_code;
                bool ok = _ctl.Auth().LoginAdminWithIdAndPassword(admin_id, password, &admin, &user, &err_code);
		response["success"] = ok;
		if (!ok)
		{
			response["error_code"] = err_code.empty() ? "LOGIN_FAILED" : err_code;
			rep.status = 401;
		}
		else
		{
			std::string session_id = _store.CreateSession(admin, user);
			rep.set_header("Set-Cookie", _store.BuildSetCookieHeader(session_id));
			response["data"]["admin_id"] = admin.admin_id;
			response["data"]["uid"] = admin.uid;
			response["data"]["role"] = admin.role;
			response["data"]["user"]["uid"] = user.uid;
			response["data"]["user"]["name"] = user.name;
			response["data"]["user"]["email"] = user.email;

		}

		Json::FastWriter writer;
		addCORSHeaders(rep);
		rep.set_content(writer.write(response), "application/json;charset=utf-8");
	});

	svr.Post("/api/admin/auth/logout", [this, addCORSHeaders](const Request& req, Response& rep) {
		std::string cookie_header = req.get_header_value("Cookie");
		_store.DestroySessionByCookie(cookie_header);
		rep.set_header("Set-Cookie", _store.BuildClearCookieHeader());

		Json::Value response;
		response["success"] = true;
		Json::FastWriter writer;
		addCORSHeaders(rep);
		rep.set_content(writer.write(response), "application/json;charset=utf-8");
	});

	svr.set_mount_point("/admin/assets", HTML_PATH + std::string("/admin"));

	svr.Get("/", [](const Request& req, Response& rep) {
		(void)req;
		rep.status = 302;
		rep.set_header("Location", "/admin/login");
	});

	svr.Get("/admin/login", [](const Request& req, Response& rep){
		(void)req;
		std::string html;
		if (!FileUtil::ReadFile(HTML_PATH+ "admin/login.html", &html))
		{
			rep.status = 500;
			rep.set_content("admin login page load failed", "text/plain;charset=utf-8");
			return;
		}
		if (html.empty())
		{
			rep.status = 500;
			rep.set_content("admin login page load failed", "text/plain;charset=utf-8");
			return;
		}
		rep.set_header("Cache-Control", "no-store, no-cache, must-revalidate, max-age=0");
		rep.set_header("Pragma", "no-cache");
		rep.set_header("Expires", "0");
		rep.set_content(html, "text/html;charset=utf-8");
	});

	auto serveAdminShell = [this, getCurrentAdmin](const Request& req, Response& rep) {
		User current_user;
		AdminAccount current_admin;
		if (!getCurrentAdmin(req, &current_user, &current_admin))
		{
			rep.status = 404;
			rep.set_content("Not Found", "text/plain;charset=utf-8");
			return;
		}
		std::string html;
		_ctl.GetStaticHtml("admin/index.html", &html);
		if (html.empty())
		{
			rep.status = 500;
			rep.set_content("admin index load failed", "text/plain;charset=utf-8");
			logger(ns_log::FATAL)<<"failed to load admin index.html!";
			return;
		}
		html = InjectUserInfo(html, &current_user, &current_admin, true);
		rep.set_header("Cache-Control", "no-store, no-cache, must-revalidate, max-age=0");
		rep.set_header("Pragma", "no-cache");
		rep.set_header("Expires", "0");
		rep.set_content(html, "text/html;charset=utf-8");
	};

	svr.Get("/admin", serveAdminShell);
	svr.Get("/admin/users", serveAdminShell);
	svr.Get("/admin/questions", serveAdminShell);
	svr.Get("/admin/questions/new", serveAdminShell);
	svr.Get(R"(/admin/questions/(\d+))", serveAdminShell);

	svr.Get("/api/admin/overview", [this, model, getCurrentAdmin, addCORSHeaders](const Request& req, Response& rep) {
		std::string request_id = BuildRequestId(req);
		constexpr const char* kOverviewCacheKey = "admin:overview:snapshot";
		constexpr int kOverviewCacheBaseTtlSeconds = 30;
		constexpr int kOverviewCacheJitterSeconds = 10;
		Json::Value response;
		User current_user;
		AdminAccount current_admin;
		if (!RequireAdmin(req, &current_user, &current_admin, getCurrentAdmin, addCORSHeaders, rep, false))
		{
			return;
		}

		std::string cached_overview_json;
		if (model->GetCachedStringByAnyKey(kOverviewCacheKey, &cached_overview_json))
		{
			Json::Value extra;
			extra["request_id"] = request_id;
			extra["cache_hit"] = true;
			TryWriteAudit(model, request_id, current_admin, "admin.overview", "system", "success", BuildPayload(req, 200, extra));

			addCORSHeaders(rep);
			rep.set_content(cached_overview_json, "application/json;charset=utf-8");
			return;
		}

		int question_count = 0;
		int user_count = 0;
		int user_pages = 0;
		int admin_count = 0;
		int super_admin_count = 0;
		int admin_role_count = 0;
		int recent_audit_total = 0;
		int recent_user_total = 0;
		std::vector<AdminAuditLog> recent_audits;
		std::vector<User> recent_users;
		std::vector<Question> all_questions;
		bool ok = BuildAdminOverview(model, &question_count, &user_count, &admin_count);
		if (ok)
		{
			KeyContext uctx;
			uctx._query = std::make_shared<UserQuery>();
			uctx.page = 1;
			uctx.size = 5;
			uctx.list_version = "1";
			uctx.list_type = ListType::Users;
			std::shared_ptr<ns_cache::Cache::CacheListKey> ukey = std::make_shared<ns_cache::Cache::CacheListKey>();
			ukey->Build(uctx);

			ok = model->GetRoleCount("super_admin", &super_admin_count)
				&& model->GetRoleCount("admin", &admin_role_count)
				&& model->ListAdminAuditLogsPaged(1, 5, "", 0, "", &recent_audits, &recent_audit_total)
				&& model->User().GetUsersPaged(ukey, &recent_users, &recent_user_total, &user_pages);
		}
		response["success"] = ok;
		if (!ok)
		{
			response["error_code"] = "LOAD_OVERVIEW_FAILED";
			rep.status = 500;
		}
		else
		{
			auto m = model->GetCacheMetricsSnapshot();
			std::sort(all_questions.begin(), all_questions.end(), [](const Question& lhs, const Question& rhs) {
				if (lhs.update_time != rhs.update_time)
				{
					return lhs.update_time > rhs.update_time;
				}
				return std::atoi(lhs.number.c_str()) > std::atoi(rhs.number.c_str());
			});

			response["data"]["question_count"] = question_count;
			response["data"]["user_count"] = user_count;
			response["data"]["admin_count"] = admin_count;
			response["data"]["admin_roles"]["super_admin"] = super_admin_count;
			response["data"]["admin_roles"]["admin"] = admin_role_count;
			response["data"]["recent_audit_total"] = recent_audit_total;
			response["data"]["cache"]["list_requests"] = Json::Int64(m.list_requests);
			response["data"]["cache"]["list_hits"] = Json::Int64(m.list_hits);
			response["data"]["cache"]["list_misses"] = Json::Int64(m.list_misses);
			response["data"]["cache"]["list_db_fallbacks"] = Json::Int64(m.list_db_fallbacks);
			response["data"]["cache"]["list_avg_ms"] = m.list_requests > 0 ? (static_cast<double>(m.list_total_ms) / static_cast<double>(m.list_requests)) : 0.0;
			response["data"]["cache"]["list_hit_rate"] = CalcRate(m.list_hits, m.list_requests);
			response["data"]["cache"]["detail_requests"] = Json::Int64(m.detail_requests);
			response["data"]["cache"]["detail_hits"] = Json::Int64(m.detail_hits);
			response["data"]["cache"]["detail_misses"] = Json::Int64(m.detail_misses);
			response["data"]["cache"]["detail_db_fallbacks"] = Json::Int64(m.detail_db_fallbacks);
			response["data"]["cache"]["detail_avg_ms"] = m.detail_requests > 0 ? (static_cast<double>(m.detail_total_ms) / static_cast<double>(m.detail_requests)) : 0.0;
			response["data"]["cache"]["detail_hit_rate"] = CalcRate(m.detail_hits, m.detail_requests);
			response["data"]["cache"]["html_static_hit_rate"] = CalcRate(m.html_static_hits, m.html_static_requests);
			response["data"]["cache"]["html_list_hit_rate"] = CalcRate(m.html_list_hits, m.html_list_requests);
			response["data"]["cache"]["html_detail_hit_rate"] = CalcRate(m.html_detail_hits, m.html_detail_requests);

			Json::Value recent_user_rows(Json::arrayValue);
			for (const auto& user : recent_users)
			{
				Json::Value item;
				item["uid"] = user.uid;
				item["name"] = user.name;
				item["email"] = user.email;
				item["create_time"] = user.create_time;
				item["last_login"] = user.last_login;
				recent_user_rows.append(item);
			}
			response["data"]["recent_users"] = recent_user_rows;

			Json::Value recent_question_rows(Json::arrayValue);
			for (size_t i = 0; i < std::min<size_t>(5, all_questions.size()); ++i)
			{
				const auto& q = all_questions[i];
				Json::Value item;
				item["number"] = q.number;
				item["title"] = q.title;
				item["star"] = q.star;
				item["update_time"] = q.update_time;
				item["create_time"] = q.create_time;
				recent_question_rows.append(item);
			}
			response["data"]["recent_questions"] = recent_question_rows;

			Json::Value recent_audit_rows(Json::arrayValue);
			for (const auto& log : recent_audits)
			{
				Json::Value item;
				item["request_id"] = log.request_id;
				item["operator_admin_id"] = log.operator_admin_id;
				item["operator_uid"] = log.operator_uid;
				item["operator_role"] = log.operator_role;
				item["action"] = log.action;
				item["resource_type"] = log.resource_type;
				item["result"] = log.result;
				item["payload_text"] = log.payload_text;
				recent_audit_rows.append(item);
			}
			response["data"]["recent_audits"] = recent_audit_rows;

			Json::FastWriter cache_writer;
			int ttl_seconds = model->BuildCacheJitteredTtlSeconds(kOverviewCacheBaseTtlSeconds, kOverviewCacheJitterSeconds);
			model->SetCachedStringByAnyKey(kOverviewCacheKey, cache_writer.write(response), ttl_seconds);
		}

		Json::Value extra;
		extra["request_id"] = request_id;
		extra["cache_hit"] = false;
		TryWriteAudit(model, request_id, current_admin, "admin.overview", "system", ok ? "success" : "failed", BuildPayload(req, rep.status == 0 ? 200 : rep.status, extra));

		Json::FastWriter writer;
		addCORSHeaders(rep);
		rep.set_content(writer.write(response), "application/json;charset=utf-8");
	});

	svr.Get("/api/admin/users", [this, model, getCurrentAdmin, addCORSHeaders](const Request& req, Response& rep) {
		std::string request_id = BuildRequestId(req);
		constexpr const char* kUsersListCachePrefix = "admin:users:list";
		constexpr int kUsersListCacheBaseTtlSeconds = 30;
		constexpr int kUsersListCacheJitterSeconds = 10;
		Json::Value response;
		User current_user;
		AdminAccount current_admin;
		if (!RequireAdmin(req, &current_user, &current_admin, getCurrentAdmin, addCORSHeaders, rep, false))
		{
			return;
		}

		int page = HttpUtil::ParsePositiveIntParam(req, "page", 1, 1, 1000000);
		int size = HttpUtil::ParsePositiveIntParam(req, "size", 20, 1, 200);
		std::string keyword = req.has_param("q") ? StringUtil::Trim(req.get_param_value("q")) : "";
		std::size_t keyword_hash = std::hash<std::string>{}(keyword);
		std::string users_cache_key = std::string(kUsersListCachePrefix)
			+ ":p:" + std::to_string(page)
			+ ":s:" + std::to_string(size)
			+ ":q:" + std::to_string(keyword_hash);
		std::string cached_users_json;
		if (model->GetCachedStringByAnyKey(users_cache_key, &cached_users_json))
		{
			Json::Value extra;
			extra["request_id"] = request_id;
			extra["page"] = page;
			extra["size"] = size;
			extra["cache_hit"] = true;
			TryWriteAudit(model, request_id, current_admin, "user.list", "user", "success", BuildPayload(req, 200, extra));

			addCORSHeaders(rep);
			rep.set_content(cached_users_json, "application/json;charset=utf-8");
			return;
		}

		std::vector<User> users;
		int total_count = 0;
		int total_pages = 0;
		bool ok = [&]() -> bool {
			auto query = std::make_shared<UserQuery>();
			if (!keyword.empty())
			{
				query->name = keyword;
				query->email = keyword;
			}
			KeyContext uctx;
			uctx._query = query;
			uctx.page = page;
			uctx.size = size;
			uctx.list_version = "1";
			uctx.list_type = ListType::Users;
			std::shared_ptr<ns_cache::Cache::CacheListKey> ukey = std::make_shared<ns_cache::Cache::CacheListKey>();
			ukey->Build(uctx);
			return model->User().GetUsersPaged(ukey, &users, &total_count, &total_pages);
		}();

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

			Json::FastWriter cache_writer;
			int ttl_seconds = model->BuildCacheJitteredTtlSeconds(kUsersListCacheBaseTtlSeconds, kUsersListCacheJitterSeconds);
			model->SetCachedStringByAnyKey(users_cache_key, cache_writer.write(response), ttl_seconds);
		}

		Json::Value extra;
		extra["request_id"] = request_id;
		extra["page"] = page;
		extra["size"] = size;
		extra["cache_hit"] = false;
		TryWriteAudit(model, request_id, current_admin, "user.list", "user", ok ? "success" : "failed", BuildPayload(req, rep.status == 0 ? 200 : rep.status, extra));

		Json::FastWriter writer;
		addCORSHeaders(rep);
		rep.set_content(writer.write(response), "application/json;charset=utf-8");
	});

	svr.Get("/api/admin/questions", [this, model, getCurrentAdmin, addCORSHeaders](const Request& req, Response& rep) {
		std::string request_id = BuildRequestId(req);
		constexpr const char* kQuestionsListCachePrefix = "admin:questions:list";
		constexpr int kQuestionsListCacheBaseTtlSeconds = 30;
		constexpr int kQuestionsListCacheJitterSeconds = 10;
		Json::Value response;
		User current_user;
		AdminAccount current_admin;
		if (!RequireAdmin(req, &current_user, &current_admin, getCurrentAdmin, addCORSHeaders, rep, false))
		{
			return;
		}

		int page = HttpUtil::ParsePositiveIntParam(req, "page", 1, 1, 1000000);
		int size = HttpUtil::ParsePositiveIntParam(req, "size", 20, 1, 200);
		std::string keyword = req.has_param("q") ? StringUtil::Trim(req.get_param_value("q")) : "";
		std::string questions_list_version = model->Question().GetQuestionsListVersion();
		std::size_t keyword_hash = std::hash<std::string>{}(keyword);
		std::string questions_cache_key = std::string(kQuestionsListCachePrefix)
			+ ":v:" + questions_list_version
			+ ":p:" + std::to_string(page)
			+ ":s:" + std::to_string(size)
			+ ":q:" + std::to_string(keyword_hash);
		std::string cached_questions_json;
		if (model->GetCachedStringByAnyKey(questions_cache_key, &cached_questions_json))
		{
			Json::Value extra;
			extra["request_id"] = request_id;
			extra["page"] = page;
			extra["size"] = size;
			extra["cache_hit"] = true;
			TryWriteAudit(model, request_id, current_admin, "question.list", "question", "success", BuildPayload(req, 200, extra));

			addCORSHeaders(rep);
			rep.set_content(cached_questions_json, "application/json;charset=utf-8");
			return;
		}

		std::vector<Question> rows;
		KeyContext context;
		auto query = std::make_shared<QuestionQuery>();
		if (!keyword.empty())
		{
			query->title = keyword;
			query->id = keyword;
		}
		context._query = query;
		context.page = page;
		context.size = size;
		context.list_version = questions_list_version;
		context.list_type = ListType::Questions;
		std::shared_ptr<ns_cache::Cache::CacheListKey> key = std::make_shared<ns_cache::Cache::CacheListKey>();
		key->Build(context);
		int total_pages = 0;
		int total_count = 0;
		bool ok = model->Question().GetQuestionsByPage(key, rows, &total_count, &total_pages);
		response["success"] = ok;
		if (!ok)
		{
			response["error_code"] = "LOAD_QUESTIONS_FAILED";
			rep.status = 500;
		}
		else
		{
			Json::Value json_rows(Json::arrayValue);
			for (const auto& q : rows)
			{
				Json::Value item;
				item["number"] = q.number;
				item["title"] = q.title;
				item["star"] = q.star;
				item["cpu_limit"] = q.cpu_limit;
				item["memory_limit"] = q.memory_limit;
				json_rows.append(item);
			}

			response["data"]["page"] = page;
			response["data"]["size"] = size;
			response["data"]["total_count"] = total_count;
			response["data"]["rows"] = json_rows;

			Json::FastWriter cache_writer;
			int ttl_seconds = model->BuildCacheJitteredTtlSeconds(kQuestionsListCacheBaseTtlSeconds, kQuestionsListCacheJitterSeconds);
			model->SetCachedStringByAnyKey(questions_cache_key, cache_writer.write(response), ttl_seconds);
		}

		Json::Value extra;
		extra["request_id"] = request_id;
		extra["page"] = page;
		extra["size"] = size;
		extra["cache_hit"] = false;
		TryWriteAudit(model, request_id, current_admin, "question.list", "question", ok ? "success" : "failed", BuildPayload(req, rep.status == 0 ? 200 : rep.status, extra));

		Json::FastWriter writer;
		addCORSHeaders(rep);
		rep.set_content(writer.write(response), "application/json;charset=utf-8");
	});

	svr.Get(R"(/api/admin/questions/(\d+))", [this, model, getCurrentAdmin, addCORSHeaders](const Request& req, Response& rep) {
		std::string request_id = BuildRequestId(req);
		Json::Value response;
		User current_user;
		AdminAccount current_admin;
		if (!RequireAdmin(req, &current_user, &current_admin, getCurrentAdmin, addCORSHeaders, rep, false))
		{
			return;
		}

		std::string number = std::string(req.matches[1]);
		Question q;
		bool ok = model->Question().GetOneQuestion(number, q);
		response["success"] = ok;
		if (!ok)
		{
			response["error_code"] = "QUESTION_NOT_FOUND";
			rep.status = 404;
		}
		else
		{
			response["data"]["number"] = q.number;
			response["data"]["title"] = q.title;
			response["data"]["desc"] = q.desc;
			response["data"]["star"] = q.star;
			response["data"]["cpu_limit"] = q.cpu_limit;
			response["data"]["memory_limit"] = q.memory_limit;
			response["data"]["create_time"] = q.create_time;
			response["data"]["update_time"] = q.update_time;
		}

		Json::Value extra;
		extra["request_id"] = request_id;
		extra["number"] = number;
		TryWriteAudit(model, request_id, current_admin, "question.get", "question", ok ? "success" : "failed", BuildPayload(req, rep.status == 0 ? 200 : rep.status, extra));

		Json::FastWriter writer;
		addCORSHeaders(rep);
		rep.set_content(writer.write(response), "application/json;charset=utf-8");
	});

	svr.Post("/api/admin/questions", [this, model, getCurrentAdmin, addCORSHeaders](const Request& req, Response& rep) {
		std::string request_id = BuildRequestId(req);
		Json::Value response;
		User current_user;
		AdminAccount current_admin;
		if (!RequireAdmin(req, &current_user, &current_admin, getCurrentAdmin, addCORSHeaders, rep, false))
		{
			return;
		}

		Json::Value in;
		if (!JsonUtil::ParseJsonBody(req, &in))
		{
			addCORSHeaders(rep);
			HttpUtil::ReplyBadRequest(rep, "请求体必须是 JSON");
			return;
		}

		Question q;
		std::string err;
		if (!BuildQuestionFromJson(in, &q, &err))
		{
			addCORSHeaders(rep);
			HttpUtil::ReplyBadRequest(rep, err);
			return;
		}

		bool ok = _ctl.Question().SaveQuestion(q);
		response["success"] = ok;
		if (!ok)
		{
			response["error_code"] = "SAVE_QUESTION_FAILED";
			rep.status = 500;
		}
		else
		{
			response["data"]["number"] = q.number;
			response["data"]["title"] = q.title;
		}
		if (ok)
		{
			model->DeleteCachedStringByAnyKey("admin:overview:snapshot");
		}

		Json::Value extra;
		extra["request_id"] = request_id;
		extra["number"] = q.number;
		extra["title"] = q.title;
		TryWriteAudit(model, request_id, current_admin, "question.save", "question", ok ? "success" : "failed", BuildPayload(req, rep.status == 0 ? 200 : rep.status, extra));

		Json::FastWriter writer;
		addCORSHeaders(rep);
		rep.set_content(writer.write(response), "application/json;charset=utf-8");
	});

	svr.Put(R"(/api/admin/questions/(\d+))", [this, model, getCurrentAdmin, addCORSHeaders](const Request& req, Response& rep) {
		std::string request_id = BuildRequestId(req);
		Json::Value response;
		User current_user;
		AdminAccount current_admin;
		if (!RequireAdmin(req, &current_user, &current_admin, getCurrentAdmin, addCORSHeaders, rep, false))
		{
			return;
		}

		Json::Value in;
		if (!JsonUtil::ParseJsonBody(req, &in))
		{
			addCORSHeaders(rep);
			HttpUtil::ReplyBadRequest(rep, "请求体必须是 JSON");
			return;
		}

		std::string number = std::string(req.matches[1]);
		Question q;
		std::string err;
		if (!BuildQuestionFromJson(in, &q, &err, &number))
		{
			addCORSHeaders(rep);
			HttpUtil::ReplyBadRequest(rep, err);
			return;
		}

		bool ok = _ctl.Question().SaveQuestion(q);
		response["success"] = ok;
		if (!ok)
		{
			response["error_code"] = "UPDATE_QUESTION_FAILED";
			rep.status = 500;
		}
		else
		{
			response["data"]["number"] = q.number;
			response["data"]["title"] = q.title;
		}
		if (ok)
		{
			model->DeleteCachedStringByAnyKey("admin:overview:snapshot");
		}

		Json::Value extra;
		extra["request_id"] = request_id;
		extra["number"] = q.number;
		extra["title"] = q.title;
		TryWriteAudit(model, request_id, current_admin, "question.update", "question", ok ? "success" : "failed", BuildPayload(req, rep.status == 0 ? 200 : rep.status, extra));

		Json::FastWriter writer;
		addCORSHeaders(rep);
		rep.set_content(writer.write(response), "application/json;charset=utf-8");
	});

	svr.Delete(R"(/api/admin/questions/(\d+))", [this, model, getCurrentAdmin, addCORSHeaders](const Request& req, Response& rep) {
		std::string request_id = BuildRequestId(req);
		Json::Value response;
		User current_user;
		AdminAccount current_admin;
		if (!RequireAdmin(req, &current_user, &current_admin, getCurrentAdmin, addCORSHeaders, rep, false))
		{
			return;
		}

		std::string number = std::string(req.matches[1]);
        bool ok = _ctl.Question().DeleteQuestion(number);
		response["success"] = ok;
		if (!ok)
		{
			response["error_code"] = "DELETE_QUESTION_FAILED";
			rep.status = 500;
		}
		if (ok)
		{
			model->DeleteCachedStringByAnyKey("admin:overview:snapshot");
		}

		Json::Value extra;
		extra["request_id"] = request_id;
		extra["number"] = number;
		TryWriteAudit(model, request_id, current_admin, "question.delete", "question", ok ? "success" : "failed", BuildPayload(req, rep.status == 0 ? 200 : rep.status, extra));

		Json::FastWriter writer;
		addCORSHeaders(rep);
		rep.set_content(writer.write(response), "application/json;charset=utf-8");
	});

	// 获取题目的所有测试用例
	svr.Get(R"(/api/admin/questions/(\d+)/tests$)", [this, model, getCurrentAdmin, addCORSHeaders](const Request& req, Response& rep) {
		Json::Value response;
		User u; AdminAccount a;
		if (!RequireAdmin(req, &u, &a, getCurrentAdmin, addCORSHeaders, rep, true)) return;

		std::string qid = req.matches[1];
		Json::Value tests;
		if (model->Question().GetTestsByQuestionId(qid, &tests)) {
			response["success"] = true;
			response["tests"] = tests["tests"];
		} else {
			response["success"] = false;
			response["error"] = "获取测试用例失败";
			rep.status = 500;
		}
		addCORSHeaders(rep);
		Json::FastWriter w; rep.set_content(w.write(response), "application/json;charset=utf-8");
	});

	// 新增测试用例
	svr.Post(R"(/api/admin/questions/(\d+)/tests$)", [this, model, getCurrentAdmin, addCORSHeaders](const Request& req, Response& rep) {
		Json::Value response;
		User u; AdminAccount a;
		if (!RequireAdmin(req, &u, &a, getCurrentAdmin, addCORSHeaders, rep, true)) return;

		Json::Value in;
		if (!JsonUtil::ParseJsonBody(req, &in)) {
			addCORSHeaders(rep); HttpUtil::ReplyBadRequest(rep, "请求体必须是 JSON"); return;
		}
		std::string qid = req.matches[1];
		std::string input = in.isMember("in") ? in["in"].asString() : "";
		std::string output = in.isMember("out") ? in["out"].asString() : "";
		bool is_sample = in.isMember("is_sample") ? in["is_sample"].asBool() : false;

		auto my = model->CreateConnection();
		if (!my) { response["success"] = false; rep.status = 500; addCORSHeaders(rep); Json::FastWriter w; rep.set_content(w.write(response), "application/json;charset=utf-8"); return; }
		std::ostringstream sql;
		sql << "insert into tests (question_id, `in`, `out`, is_sample) values ('"
			<< qid << "', '" << model->EscapeSqlString(input, my.get()) << "', '"
			<< model->EscapeSqlString(output, my.get()) << "', " << (is_sample ? 1 : 0) << ")";
		if (mysql_query(my.get(), sql.str().c_str()) == 0) {
			response["success"] = true;
			response["test_id"] = static_cast<Json::UInt64>(mysql_insert_id(my.get()));
		} else {
			response["success"] = false;
			response["error"] = "新增测试用例失败";
		}
		addCORSHeaders(rep);
		Json::FastWriter w; rep.set_content(w.write(response), "application/json;charset=utf-8");
	});

	// 编辑测试用例
	svr.Put(R"(/api/admin/tests/(\d+)$)", [this, model, getCurrentAdmin, addCORSHeaders](const Request& req, Response& rep) {
		Json::Value response;
		User u; AdminAccount a;
		if (!RequireAdmin(req, &u, &a, getCurrentAdmin, addCORSHeaders, rep, true)) return;

		Json::Value in;
		if (!JsonUtil::ParseJsonBody(req, &in)) {
			addCORSHeaders(rep); HttpUtil::ReplyBadRequest(rep, "请求体必须是 JSON"); return;
		}
		int test_id = std::stoi(req.matches[1]);
		auto my = model->CreateConnection();
		if (!my) { response["success"] = false; rep.status = 500; addCORSHeaders(rep); Json::FastWriter w; rep.set_content(w.write(response), "application/json;charset=utf-8"); return; }
		std::ostringstream sql;
		sql << "update tests set ";
		bool first = true;
		if (in.isMember("in")) { sql << "`in`='" << model->EscapeSqlString(in["in"].asString(), my.get()) << "'"; first = false; }
		if (in.isMember("out")) { if (!first) sql << ", "; sql << "`out`='" << model->EscapeSqlString(in["out"].asString(), my.get()) << "'"; first = false; }
		if (in.isMember("is_sample")) { if (!first) sql << ", "; sql << "is_sample=" << (in["is_sample"].asBool() ? 1 : 0); }
		sql << " where id=" << test_id;
		if (mysql_query(my.get(), sql.str().c_str()) == 0)
			response["success"] = true;
		else { response["success"] = false; response["error"] = "编辑测试用例失败"; }
		addCORSHeaders(rep);
		Json::FastWriter w; rep.set_content(w.write(response), "application/json;charset=utf-8");
	});

	// 删除测试用例
	svr.Delete(R"(/api/admin/tests/(\d+)$)", [this, model, getCurrentAdmin, addCORSHeaders](const Request& req, Response& rep) {
		Json::Value response;
		User u; AdminAccount a;
		if (!RequireAdmin(req, &u, &a, getCurrentAdmin, addCORSHeaders, rep, true)) return;
		int test_id = std::stoi(req.matches[1]);
		auto my = model->CreateConnection();
		if (!my) { response["success"] = false; rep.status = 500; addCORSHeaders(rep); Json::FastWriter w; rep.set_content(w.write(response), "application/json;charset=utf-8"); return; }
		std::string sql = "delete from tests where id=" + std::to_string(test_id);
		if (mysql_query(my.get(), sql.c_str()) == 0)
			response["success"] = true;
		else { response["success"] = false; response["error"] = "删除测试用例失败"; }
		addCORSHeaders(rep);
		Json::FastWriter w; rep.set_content(w.write(response), "application/json;charset=utf-8");
	});

	svr.Options("/api/admin/questions/cache/invalidate", [addCORSHeaders](const Request& req, Response& rep) {
		(void)req; addCORSHeaders(rep); rep.status = 204; });

	svr.Post("/api/admin/questions/cache/invalidate", [this, model, getCurrentAdmin, addCORSHeaders](const Request& req, Response& rep) {
		(void)req;
		Json::Value response;

		AdminAccount admin;
		if (!getCurrentAdmin(req, nullptr, &admin))
		{
			response["success"] = false;
			response["error"] = "未登录";
			Json::FastWriter writer;
			addCORSHeaders(rep);
			rep.status = 401;
			rep.set_content(writer.write(response), "application/json;charset=utf-8");
			return;
		}

                _ctl.Question().TouchQuestionListVersion();
		response["success"] = true;
		response["operator"] = admin.uid;

		logger(INFO) << "[admin][cache] list version invalidated by admin_id=" << admin.admin_id;

		Json::FastWriter writer;
		addCORSHeaders(rep);
		rep.set_content(writer.write(response), "application/json;charset=utf-8");
	});

	svr.Get("/api/admin/accounts", [this, model, getCurrentAdmin, addCORSHeaders](const Request& req, Response& rep) {
		std::string request_id = BuildRequestId(req);
		Json::Value response;
		User current_user;
		AdminAccount current_admin;
		if (!RequireAdmin(req, &current_user, &current_admin, getCurrentAdmin, addCORSHeaders, rep, true))
		{
			return;
		}

		int page = HttpUtil::ParsePositiveIntParam(req, "page", 1, 1, 1000000);
		int size = HttpUtil::ParsePositiveIntParam(req, "size", 20, 1, 200);
		std::string keyword = req.has_param("q") ? StringUtil::Trim(req.get_param_value("q")) : "";

		std::vector<AdminAccount> admins;
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

	svr.Get(R"(/api/admin/accounts/(\d+))", [this, model, getCurrentAdmin, addCORSHeaders](const Request& req, Response& rep) {
		std::string request_id = BuildRequestId(req);
		Json::Value response;
		User current_user;
		AdminAccount current_admin;
		if (!RequireAdmin(req, &current_user, &current_admin, getCurrentAdmin, addCORSHeaders, rep, true))
		{
			return;
		}

		int admin_id = std::atoi(std::string(req.matches[1]).c_str());
		AdminAccount target;
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

	svr.Post("/api/admin/accounts", [this, model, getCurrentAdmin, addCORSHeaders](const Request& req, Response& rep) {
		std::string request_id = BuildRequestId(req);
		Json::Value response;
		User current_user;
		AdminAccount current_admin;
		if (!RequireAdmin(req, &current_user, &current_admin, getCurrentAdmin, addCORSHeaders, rep, true))
		{
			return;
		}

		Json::Value in;
		if (!JsonUtil::ParseJsonBody(req, &in))
		{
			addCORSHeaders(rep);
			HttpUtil::ReplyBadRequest(rep, "请求体必须是 JSON");
			return;
		}

		if (!in.isMember("uid") || !in["uid"].isInt())
		{
			addCORSHeaders(rep);
			HttpUtil::ReplyBadRequest(rep, "uid 必须是整数");
			return;
		}
		int uid = in["uid"].asInt();
		std::string role = in.isMember("role") && in["role"].isString() ? StringUtil::Trim(in["role"].asString()) : "admin";
		std::string password = in.isMember("password") && in["password"].isString() ? in["password"].asString() : "";

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

		User target_user;
		if (!this->_ctl.GetModel()->User().GetUserById(uid, &target_user))
		{
			response["success"] = false;
			response["error_code"] = "USER_NOT_FOUND";
			rep.status = 404;
			Json::FastWriter writer;
			addCORSHeaders(rep);
			rep.set_content(writer.write(response), "application/json;charset=utf-8");
			return;
		}

		AdminAccount existing;
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

		std::string password_hash;
		std::string hash_err;
                if (!_ctl.Auth().BuildSecurePasswordHash(password, &password_hash, &hash_err))
		{
			response["success"] = false;
			response["error_code"] = hash_err.empty() ? "PASSWORD_HASH_FAILED" : hash_err;
			rep.status = 400;
			Json::FastWriter writer;
			addCORSHeaders(rep);
			rep.set_content(writer.write(response), "application/json;charset=utf-8");
			return;
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

	svr.Put(R"(/api/admin/accounts/(\d+))", [this, model, getCurrentAdmin, addCORSHeaders](const Request& req, Response& rep) {
		std::string request_id = BuildRequestId(req);
		Json::Value response;
		User current_user;
		AdminAccount current_admin;
		if (!RequireAdmin(req, &current_user, &current_admin, getCurrentAdmin, addCORSHeaders, rep, true))
		{
			return;
		}

		int target_admin_id = std::atoi(std::string(req.matches[1]).c_str());
		AdminAccount target;
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
		if (!JsonUtil::ParseJsonBody(req, &in))
		{
			addCORSHeaders(rep);
			HttpUtil::ReplyBadRequest(rep, "请求体必须是 JSON");
			return;
		}

		if (!in.isMember("role") || !in["role"].isString())
		{
			addCORSHeaders(rep);
			HttpUtil::ReplyBadRequest(rep, "role 不能为空");
			return;
		}
		std::string new_role = StringUtil::Trim(in["role"].asString());
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

	svr.Delete(R"(/api/admin/accounts/(\d+))", [this, model, getCurrentAdmin, addCORSHeaders](const Request& req, Response& rep) {
		std::string request_id = BuildRequestId(req);
		Json::Value response;
		User current_user;
		AdminAccount current_admin;
		if (!RequireAdmin(req, &current_user, &current_admin, getCurrentAdmin, addCORSHeaders, rep, true))
		{
			return;
		}

		int target_admin_id = std::atoi(std::string(req.matches[1]).c_str());
		AdminAccount target;
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

	svr.Get("/api/admin/audit/logs", [this, model, getCurrentAdmin, addCORSHeaders](const Request& req, Response& rep) {
		std::string request_id = BuildRequestId(req);
		Json::Value response;
		User current_user;
		AdminAccount current_admin;
		if (!RequireAdmin(req, &current_user, &current_admin, getCurrentAdmin, addCORSHeaders, rep, false))
		{
			return;
		}

		int page = HttpUtil::ParsePositiveIntParam(req, "page", 1, 1, 1000000);
		int size = HttpUtil::ParsePositiveIntParam(req, "size", 20, 1, 200);
		int operator_uid = HttpUtil::ParsePositiveIntParam(req, "operator_uid", 0, 0, 2147483647);
		std::string action = req.has_param("action") ? StringUtil::Trim(req.get_param_value("action")) : "";
		std::string result = req.has_param("result") ? StringUtil::Trim(req.get_param_value("result")) : "";

		std::vector<AdminAuditLog> logs;
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


	svr.Options("/api/admin/overview", [addCORSHeaders](const Request& req, Response& rep) {
		(void)req;
		addCORSHeaders(rep);
		rep.status = 200;
	});

	svr.Options("/api/admin/users", [addCORSHeaders](const Request& req, Response& rep) {
		(void)req;
		addCORSHeaders(rep);
		rep.status = 200;
	});

	svr.Options("/api/admin/questions", [addCORSHeaders](const Request& req, Response& rep) {
		(void)req;
		addCORSHeaders(rep);
		rep.status = 200;
	});

	svr.Options(R"(/api/admin/questions/(\d+))", [addCORSHeaders](const Request& req, Response& rep) {
		(void)req;
		addCORSHeaders(rep);
		rep.status = 200;
	});

	svr.Options("/api/admin/version", [addCORSHeaders](const Request& req, Response& rep) {
		(void)req;
		addCORSHeaders(rep);
		rep.status = 200;
	});

	svr.Options("/api/admin/auth/login", [addCORSHeaders](const Request& req, Response& rep) {
		(void)req;
		addCORSHeaders(rep);
		rep.status = 200;
	});

	svr.Options("/api/admin/auth/logout", [addCORSHeaders](const Request& req, Response& rep) {
		(void)req;
		addCORSHeaders(rep);
		rep.status = 200;
	});

	svr.Options("/api/admin/accounts", [addCORSHeaders](const Request& req, Response& rep) {
		(void)req;
		addCORSHeaders(rep);
		rep.status = 200;
	});

	svr.Options(R"(/api/admin/accounts/(\d+))", [addCORSHeaders](const Request& req, Response& rep) {
		(void)req;
		addCORSHeaders(rep);
		rep.status = 200;
	});

	svr.Options("/api/admin/audit/logs", [addCORSHeaders](const Request& req, Response& rep) {
		(void)req;
		addCORSHeaders(rep);
		rep.status = 200;
	});

	svr.Options("/api/*", [addCORSHeaders](const Request& req, Response& rep) {
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