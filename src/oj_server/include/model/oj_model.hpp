#pragma once

#include <cstdio>
#include <assert.h>
#include <cstdlib>
#include <vector>
#include <memory>
#include <algorithm>
#include <cctype>
#include <sstream>
#include <chrono>
#include <atomic>
#include <map>
#include <Logger/logstrategy.h>
#include "../../../comm/comm.hpp"
#include "../oj_cache.hpp"
#include "model_base.hpp"
#include "model_comment.hpp"
#include "model_solution.hpp"
#include "model_user.hpp"
#include "model_question.hpp"
#include <mysql/mysql.h>

// model: 主要用来和数据进行交互，对外提供访问数据的接口

namespace ns_model
{
    using namespace ns_log;
    using namespace ns_cache;
    //用户属性
    class Model : public ModelBase
    {
    private:
        struct CacheMetrics// 缓存指标结构体:记录题目列表和题目详情这两条查询链路的运行指标。
        {
            std::atomic<long long> list_requests{0};// all_questions请求总数
            std::atomic<long long> list_hits{0};// all_questions缓存命中数
            std::atomic<long long> list_misses{0};// all_questions缓存未命中数
            std::atomic<long long> list_db_fallbacks{0};// all_questions数据库回退数
            std::atomic<long long> list_total_ms{0};// all_questions总耗时

            std::atomic<long long> detail_requests{0};// one_question请求总数
            std::atomic<long long> detail_hits{0};// one_question缓存命中数
            std::atomic<long long> detail_misses{0};// one_question缓存未命中数
            std::atomic<long long> detail_db_fallbacks{0};// one_question数据库回退数
            std::atomic<long long> detail_total_ms{0};// one_question总耗时

            std::atomic<long long> html_static_requests{0};
            std::atomic<long long> html_static_hits{0};
            std::atomic<long long> html_static_misses{0};

            std::atomic<long long> html_list_requests{0};
            std::atomic<long long> html_list_hits{0};
            std::atomic<long long> html_list_misses{0};

            std::atomic<long long> html_detail_requests{0};
            std::atomic<long long> html_detail_hits{0};
            std::atomic<long long> html_detail_misses{0};
        };

        static CacheMetrics& Metrics()// 获取缓存指标实例
        {
            static CacheMetrics m;
            return m;
        }

        // 记录题目列表查询的指标数据
        void RecordListMetrics(bool cache_hit, bool db_fallback, long long cost_ms)
        {
            auto& m = Metrics();
            long long req = m.list_requests.fetch_add(1, std::memory_order_relaxed) + 1;
            if (cache_hit)
                m.list_hits.fetch_add(1, std::memory_order_relaxed);
            else
                m.list_misses.fetch_add(1, std::memory_order_relaxed);
            if (db_fallback)
                m.list_db_fallbacks.fetch_add(1, std::memory_order_relaxed);
            m.list_total_ms.fetch_add(cost_ms, std::memory_order_relaxed);

            if (req % 20 == 0)
            {
                long long hits = m.list_hits.load(std::memory_order_relaxed);
                long long misses = m.list_misses.load(std::memory_order_relaxed);
                long long fallbacks = m.list_db_fallbacks.load(std::memory_order_relaxed);
                long long total_ms = m.list_total_ms.load(std::memory_order_relaxed);
                double avg_ms = req == 0 ? 0.0 : static_cast<double>(total_ms) / static_cast<double>(req);
                logger(ns_log::INFO) << "[metrics][all_questions] req=" << req
                                     << " hit=" << hits
                                     << " miss=" << misses
                                     << " db_fallback=" << fallbacks
                                     << " avg_ms=" << avg_ms;
            }
        }

        // 记录题目详情查询的指标数据  
        void RecordDetailMetrics(bool cache_hit, bool db_fallback, long long cost_ms)
        {
            auto& m = Metrics();
            long long req = m.detail_requests.fetch_add(1, std::memory_order_relaxed) + 1;
            if (cache_hit)
                m.detail_hits.fetch_add(1, std::memory_order_relaxed);
            else
                m.detail_misses.fetch_add(1, std::memory_order_relaxed);
            if (db_fallback)
                m.detail_db_fallbacks.fetch_add(1, std::memory_order_relaxed);
            m.detail_total_ms.fetch_add(cost_ms, std::memory_order_relaxed);

            if (req % 20 == 0)
            {
                long long hits = m.detail_hits.load(std::memory_order_relaxed);
                long long misses = m.detail_misses.load(std::memory_order_relaxed);
                long long fallbacks = m.detail_db_fallbacks.load(std::memory_order_relaxed);
                long long total_ms = m.detail_total_ms.load(std::memory_order_relaxed);
                double avg_ms = req == 0 ? 0.0 : static_cast<double>(total_ms) / static_cast<double>(req);
                logger(ns_log::INFO) << "[metrics][one_question] req=" << req
                                     << " hit=" << hits
                                     << " miss=" << misses
                                     << " db_fallback=" << fallbacks
                                     << " avg_ms=" << avg_ms;
            }
        }
        // 记录静态HTML页面查询的指标数据
        void RecordHtmlStaticMetrics(bool cache_hit)
        {
            auto& m = Metrics();
            m.html_static_requests.fetch_add(1, std::memory_order_relaxed);
            if (cache_hit)
                m.html_static_hits.fetch_add(1, std::memory_order_relaxed);
            else
                m.html_static_misses.fetch_add(1, std::memory_order_relaxed);
        }

        void RecordHtmlListMetrics(bool cache_hit)
        {
            auto& m = Metrics();
            m.html_list_requests.fetch_add(1, std::memory_order_relaxed);
            if (cache_hit)
                m.html_list_hits.fetch_add(1, std::memory_order_relaxed);
            else
                m.html_list_misses.fetch_add(1, std::memory_order_relaxed);
        }

        void RecordHtmlDetailMetrics(bool cache_hit)
        {
            auto& m = Metrics();
            m.html_detail_requests.fetch_add(1, std::memory_order_relaxed);
            if (cache_hit)
                m.html_detail_hits.fetch_add(1, std::memory_order_relaxed);
            else
                m.html_detail_misses.fetch_add(1, std::memory_order_relaxed);
        }

    private:

        bool QueryAdminAccount(const std::string& sql, AdminAccount* admin)
        {
            if (admin == nullptr)
            {
                return false;
            }

            auto my = CreateConnection();
            if (!my)
                return false;

            MYSQL_RES* res = ModelBase::QueryMySql(my.get(), sql, "MySql管理员查询错误");
            if (!res) return false;

            int rows = mysql_num_rows(res);
            if (rows != 1)
            {
                mysql_free_result(res);
                return false;
            }

            MYSQL_ROW row = mysql_fetch_row(res);
            if (row == nullptr)
            {
                mysql_free_result(res);
                return false;
            }

            admin->admin_id = row[0] == nullptr ? 0 : std::atoi(row[0]);
            admin->password_hash = row[1] == nullptr ? "" : row[1];
            admin->uid = row[2] == nullptr ? 0 : std::atoi(row[2]);
            admin->role = row[3] == nullptr ? "" : row[3];
            admin->created_at = row[4] == nullptr ? "" : row[4];
            mysql_free_result(res);
            return true;
        }

        bool QueryAdminAccounts(const std::string& sql, std::vector<AdminAccount>* admins)
        {
            if (admins == nullptr)
            {
                return false;
            }

            auto my = CreateConnection();
            if (!my)
                return false;

            MYSQL_RES* res = ModelBase::QueryMySql(my.get(), sql, "MySql管理员列表查询错误");
            if (!res) return false;

            int rows = mysql_num_rows(res);
            admins->clear();
            admins->reserve(rows);
            for (int i = 0; i < rows; ++i)
            {
                MYSQL_ROW row = mysql_fetch_row(res);
                if (row == nullptr)
                {
                    continue;
                }

                AdminAccount admin;
                admin.admin_id = row[0] == nullptr ? 0 : std::atoi(row[0]);
                admin.password_hash = row[1] == nullptr ? "" : row[1];
                admin.uid = row[2] == nullptr ? 0 : std::atoi(row[2]);
                admin.role = row[3] == nullptr ? "" : row[3];
                admin.created_at = row[4] == nullptr ? "" : row[4];
                admins->push_back(admin);
            }

            mysql_free_result(res);
            return true;
        }

        bool QueryAuditLogs(const std::string& sql, std::vector<AdminAuditLog>* logs)
        {
            if (logs == nullptr)
            {
                return false;
            }

            auto my = CreateConnection();
            if (!my)
                return false;

            MYSQL_RES* res = ModelBase::QueryMySql(my.get(), sql, "MySql审计日志查询错误");
            if (!res) return false;

            int rows = mysql_num_rows(res);
            logs->clear();
            logs->reserve(rows);
            for (int i = 0; i < rows; ++i)
            {
                MYSQL_ROW row = mysql_fetch_row(res);
                if (row == nullptr)
                {
                    continue;
                }

                AdminAuditLog item;
                item.request_id = row[0] == nullptr ? "" : row[0];
                item.operator_admin_id = row[1] == nullptr ? 0 : std::atoi(row[1]);
                item.operator_uid = row[2] == nullptr ? 0 : std::atoi(row[2]);
                item.operator_role = row[3] == nullptr ? "" : row[3];
                item.action = row[4] == nullptr ? "" : row[4];
                item.resource_type = row[5] == nullptr ? "" : row[5];
                item.result = row[6] == nullptr ? "" : row[6];
                item.payload_text = row[7] == nullptr ? "" : row[7];
                logs->push_back(item);
            }

            mysql_free_result(res);
            return true;
        }

    public:
        ModelComment  _comment;
        ModelSolution _solution;
        ModelUser     _user;
        ModelQuestion _question;
        
        ModelComment&  Comment()  { return _comment; }
        ModelSolution& Solution() { return _solution; }
        ModelUser&     User()     { return _user; }
        ModelQuestion& Question() { return _question; }

        const std::string& AdminAccountsTable() const
        {
            static const std::string table = "admin_accounts";
            return table;
        }

        const std::string& AdminAuditLogsTable() const
        {
            static const std::string table = "admin_audit_logs";
            return table;
        }

        struct CacheMetricsSnapshot
        {
            long long list_requests = 0;
            long long list_hits = 0;
            long long list_misses = 0;
            long long list_db_fallbacks = 0;
            long long list_total_ms = 0;

            long long detail_requests = 0;
            long long detail_hits = 0;
            long long detail_misses = 0;
            long long detail_db_fallbacks = 0;
            long long detail_total_ms = 0;

            long long html_static_requests = 0;
            long long html_static_hits = 0;
            long long html_static_misses = 0;

            long long html_list_requests = 0;
            long long html_list_hits = 0;
            long long html_list_misses = 0;

            long long html_detail_requests = 0;
            long long html_detail_hits = 0;
            long long html_detail_misses = 0;
        };

        // 在cache中获取静态HTML页面
        bool GetHtmlPage(std::string *html,
                         std::shared_ptr<Cache::CacheKey> key)
        {
            return _cache.GetHtmlPage(html, key);
        }
        void SetHtmlPage(std::string *html,
                         std::shared_ptr<Cache::CacheKey> key)
        {
            _cache.SetHtmlPage(html, key);
        }

        bool GetCachedStringByAnyKey(const std::string& key, std::string* value)
        {
            if (value == nullptr)
            {
                return false;
            }
            return _cache.GetStringByAnyKey(key, value);
        }

        bool SetCachedStringByAnyKey(const std::string& key, const std::string& value, int expire_seconds)
        {
            if (expire_seconds <= 0)
            {
                return false;
            }
            return _cache.SetStringByAnyKey(key, value, expire_seconds);
        }

        bool DeleteCachedStringByAnyKey(const std::string& key)
        {
            if (key.empty())
            {
                return false;
            }
            return _cache.DeleteStringByAnyKey(key);
        }

        int BuildCacheJitteredTtlSeconds(int base_seconds, int jitter_seconds)
        {
            return _cache.BuildJitteredTtl(base_seconds, jitter_seconds);
        }
        void InvalidatePage(std::shared_ptr<Cache::CacheKey> key)
        {
            _cache.InvalidatePage(key);
        }
        void RecordStaticHtmlCacheMetrics(bool cache_hit)
        {
            RecordHtmlStaticMetrics(cache_hit);
        }
        void RecordListHtmlCacheMetrics(bool cache_hit)
        {
            RecordHtmlListMetrics(cache_hit);
        }
        void RecordDetailHtmlCacheMetrics(bool cache_hit)
        {
            RecordHtmlDetailMetrics(cache_hit);
        }


        // Admin: check if a uid belongs to an admin account
        bool GetAdminByUid(int uid)
        {
            AdminAccount admin;
            std::string sql = "select admin_id,password_hash,uid,role,created_at from " + AdminAccountsTable() + " where uid=" + std::to_string(uid);
            return QueryAdminAccount(sql, &admin);
        }

        bool GetAdminByUid(int uid, AdminAccount* admin)
        {
            std::string sql = "select admin_id,password_hash,uid,role,created_at from " + AdminAccountsTable() + " where uid=" + std::to_string(uid);
            return QueryAdminAccount(sql, admin);
        }

        bool GetAdminByAdminId(int admin_id, AdminAccount* admin)
        {
            std::string sql = "select admin_id,password_hash,uid,role,created_at from " + AdminAccountsTable() + " where admin_id=" + std::to_string(admin_id);
            return QueryAdminAccount(sql, admin);
        }

        bool GetAdminCount(int* count)
        {
            if(count == nullptr)
            {
                return false;
            }
            std::string key = "admin_counts";
            std::string value;
            if(_cache.GetStringByAnyKey(key, &value))
            {
                *count = std::atoi(value.c_str());
                logger(ns_log::INFO) << "Cache hit for admin count";
                return true;
            }
            std::string sql = "select count(*) from " + AdminAccountsTable();
            if(!QueryCount(sql, count))
            {
                return false;
             }
             _cache.SetStringByAnyKey(key, std::to_string(*count), _cache.BuildJitteredTtl(6 * 60 * 60, 30 * 60));
             return true;
        }

        bool GetRoleCount(const std::string& role, int* count)
        {
            if (count == nullptr)
            {
                return false;
            }
            std::string value;
            if(_cache.GetStringByAnyKey(role, &value))
            {
                *count = std::atoi(value.c_str());
                logger(ns_log::INFO) << "Cache hit for role count of role " << role;
                return true;
            }
            auto my = CreateConnection();
            if (!my)
            {
                return false;
            }

            std::string safe_role = EscapeSqlString(role, my.get());
            std::string sql = "select count(*) from " + AdminAccountsTable() + " where role='" + safe_role + "'";
            if(!QueryCount(sql, count))
            {
                return false;
             }
             _cache.SetStringByAnyKey(role, std::to_string(*count), _cache.BuildJitteredTtl(6 * 60 * 60, 30 * 60));
             return true;
        }

        bool ListAdminsPaged(int page, int size, const std::string& keyword, std::vector<AdminAccount>* admins, int* total_count)
        {
            if (admins == nullptr || total_count == nullptr)
            {
                return false;
            }

            int safe_page = std::max(1, page);
            int safe_size = std::max(1, std::min(size, 200));
            int offset = (safe_page - 1) * safe_size;

            auto my = CreateConnection();
            if (!my)
            {
                return false;
            }

            std::string where_clause;
            std::string trimmed_keyword = TrimCopy(keyword);
            if (!trimmed_keyword.empty())
            {
                std::string safe_kw = EscapeSqlString(trimmed_keyword, my.get());
                if (IsAllDigits(trimmed_keyword))
                {
                    where_clause = " where uid=" + trimmed_keyword + " or role like '%" + safe_kw + "%'";
                }
                else
                {
                    where_clause = " where role like '%" + safe_kw + "%'";
                }
            }

            std::string count_sql = "select count(*) from " + AdminAccountsTable() + where_clause;
            if (!QueryCount(count_sql, total_count))
            {
                return false;
            }

            std::ostringstream sql;
            sql << "select admin_id,password_hash,uid,role,created_at from " << AdminAccountsTable()
                << where_clause
                << " order by admin_id desc limit " << safe_size << " offset " << offset;
            return QueryAdminAccounts(sql.str(), admins);
        }

        bool CreateAdminAccount(int uid, const std::string& password_hash, const std::string& role)
        {
            auto my = CreateConnection();
            if (!my)
            {
                return false;
            }

            std::string safe_hash = EscapeSqlString(password_hash, my.get());
            std::string safe_role = EscapeSqlString(role, my.get());
            std::string sql = "insert into " + AdminAccountsTable() +
                              " (password_hash, uid, role, created_at) values ('" + safe_hash + "', " + std::to_string(uid) + ", '" + safe_role + "', NOW())";
            return ExecuteSql(sql);
        }

        bool UpdateAdminRole(int admin_id, const std::string& new_role)
        {
            auto my = CreateConnection();
            if (!my)
            {
                return false;
            }

            std::string safe_role = EscapeSqlString(new_role, my.get());
            std::string sql = "update " + AdminAccountsTable() + " set role='" + safe_role + "' where admin_id=" + std::to_string(admin_id);
            return ExecuteSql(sql);
        }

        bool DeleteAdminAccount(int admin_id)
        {
            std::string sql = "delete from " + AdminAccountsTable() + " where admin_id=" + std::to_string(admin_id);
            return ExecuteSql(sql);
        }

        bool InsertAdminAuditLog(const AdminAuditLog& log)
        {
            auto my = CreateConnection();
            if (!my)
            {
                logger(ns_log::FATAL) << "无法连接数据库，无法记录审计日志!";
                return false;
            }

            std::string safe_request_id = EscapeSqlString(log.request_id, my.get());
            std::string safe_role = EscapeSqlString(log.operator_role, my.get());
            std::string safe_action = EscapeSqlString(log.action, my.get());
            std::string safe_resource_type = EscapeSqlString(log.resource_type, my.get());
            std::string safe_result = EscapeSqlString(log.result, my.get());
            std::string safe_payload = EscapeSqlString(log.payload_text, my.get());

            std::ostringstream sql;
            sql << "insert into " << AdminAuditLogsTable()
                << " (request_id, operator_admin_id, operator_uid, operator_role, action, resource_type, result, created_at, payload_text) values ('"
                << safe_request_id << "', " << log.operator_admin_id << ", " << log.operator_uid << ", '"
                << safe_role << "', '" << safe_action << "', '" << safe_resource_type << "', '"
                << safe_result << "', NOW(), '" << safe_payload << "')";

            return ExecuteSql(sql.str());
        }

        bool ListAdminAuditLogsPaged(int page,
                                     int size,
                                     const std::string& action,
                                     int operator_uid,
                                     const std::string& result,
                                     std::vector<AdminAuditLog>* logs,
                                     int* total_count)
        {
            if (logs == nullptr || total_count == nullptr)
            {
                return false;
            }

            int safe_page = std::max(1, page);
            int safe_size = std::max(1, std::min(size, 200));
            int offset = (safe_page - 1) * safe_size;

            auto my = CreateConnection();
            if (!my)
            {
                return false;
            }

            std::vector<std::string> clauses;
            if (!action.empty())
            {
                std::string safe_action = EscapeSqlString(action, my.get());
                clauses.push_back("action='" + safe_action + "'");
            }
            if (operator_uid > 0)
            {
                clauses.push_back("operator_uid=" + std::to_string(operator_uid));
            }
            if (!result.empty())
            {
                std::string safe_result = EscapeSqlString(result, my.get());
                clauses.push_back("result='" + safe_result + "'");
            }

            std::string where_clause;
            if (!clauses.empty())
            {
                std::ostringstream where;
                where << " where ";
                for (size_t i = 0; i < clauses.size(); ++i)
                {
                    if (i != 0)
                    {
                        where << " and ";
                    }
                    where << clauses[i];
                }
                where_clause = where.str();
            }

            std::string count_sql = "select count(*) from " + AdminAuditLogsTable() + where_clause;
            if (!QueryCount(count_sql, total_count))
            {
                return false;
            }

            std::ostringstream sql;
            sql << "select request_id,operator_admin_id,operator_uid,operator_role,action,resource_type,result,payload_text from "
                << AdminAuditLogsTable()
                << where_clause
                << " order by log_id desc limit " << safe_size << " offset " << offset;

            return QueryAuditLogs(sql.str(), logs);
        }

        CacheMetricsSnapshot GetCacheMetricsSnapshot() const
        {
            const auto& m = Metrics();
            CacheMetricsSnapshot s;
            s.list_requests = m.list_requests.load(std::memory_order_relaxed);
            s.list_hits = m.list_hits.load(std::memory_order_relaxed);
            s.list_misses = m.list_misses.load(std::memory_order_relaxed);
            s.list_db_fallbacks = m.list_db_fallbacks.load(std::memory_order_relaxed);
            s.list_total_ms = m.list_total_ms.load(std::memory_order_relaxed);

            s.detail_requests = m.detail_requests.load(std::memory_order_relaxed);
            s.detail_hits = m.detail_hits.load(std::memory_order_relaxed);
            s.detail_misses = m.detail_misses.load(std::memory_order_relaxed);
            s.detail_db_fallbacks = m.detail_db_fallbacks.load(std::memory_order_relaxed);
            s.detail_total_ms = m.detail_total_ms.load(std::memory_order_relaxed);

            s.html_static_requests = m.html_static_requests.load(std::memory_order_relaxed);
            s.html_static_hits = m.html_static_hits.load(std::memory_order_relaxed);
            s.html_static_misses = m.html_static_misses.load(std::memory_order_relaxed);

            s.html_list_requests = m.html_list_requests.load(std::memory_order_relaxed);
            s.html_list_hits = m.html_list_hits.load(std::memory_order_relaxed);
            s.html_list_misses = m.html_list_misses.load(std::memory_order_relaxed);

            s.html_detail_requests = m.html_detail_requests.load(std::memory_order_relaxed);
            s.html_detail_hits = m.html_detail_hits.load(std::memory_order_relaxed);
            s.html_detail_misses = m.html_detail_misses.load(std::memory_order_relaxed);
            return s;
        }

    private:
    };

};
