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
#include <thread>

// model: 主要用来和数据进行交互，对外提供访问数据的接口

namespace ns_model
{
    using namespace ns_log;
    using namespace ns_cache;
    //用户属性
    class Model : public ModelBase
    {
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
                item.operator_role = row[2] == nullptr ? "" : row[2];
                item.action = row[3] == nullptr ? "" : row[3];
                item.resource_type = row[4] == nullptr ? "" : row[4];
                item.result = row[5] == nullptr ? "" : row[5];
                item.payload_text = row[6] == nullptr ? "" : row[6];
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
                << " (request_id, operator_admin_id, operator_role, action, resource_type, result, created_at, payload_text) values ('"
                << safe_request_id << "', " << log.operator_admin_id << ", '"
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
            sql << "select request_id,operator_admin_id,operator_role,action,resource_type,result,payload_text from "
                << AdminAuditLogsTable()
                << where_clause
                << " order by log_id desc limit " << safe_size << " offset " << offset;

            return QueryAuditLogs(sql.str(), logs);
        }

        void InsertCacheMetricSnapshot(int action_type, long long total, long long hits, long long falls, long long ms)
        {
            auto my = CreateConnection();
            if (!my) return;
            std::ostringstream sql;
            sql << "insert into cache_metrics_snapshots (action_type,total_requests,cache_hits,db_fallbacks,total_ms) values ("
                << action_type << "," << total << "," << hits << "," << falls << "," << ms << ")";
            mysql_query(my.get(), sql.str().c_str()); // best-effort
        }

        void FlushCacheMetrics()
        {
            for (int t = 0; t < 5; ++t)
            {
                auto& m = Metrics().actions[t];
                long long req = m.total_requests.exchange(0);
                if (req == 0) continue;
                long long hits = m.cache_hits.exchange(0);
                long long falls = m.db_fallbacks.exchange(0);
                long long ms = m.total_ms.exchange(0);
                InsertCacheMetricSnapshot(t, req, hits, falls, ms);
            }
        }

        void StartMetricsFlushWorker()
        {
            _metrics_running = true;
            _metrics_worker = std::thread([this]() {
                while (_metrics_running)
                {
                    for (int i = 0; i < 30; ++i)
                    {
                        if (!_metrics_running) return;
                        bool need_flush = false;
                        for (int t = 0; t < 5; ++t)
                            if (Metrics().actions[t].total_requests.load() >= 100) { need_flush = true; break; }
                        if (need_flush) break;
                        std::this_thread::sleep_for(std::chrono::seconds(1));
                    }
                    if (!_metrics_running) return;
                    FlushCacheMetrics();
                }
            });
        }

        void StopMetricsFlushWorker()
        {
            _metrics_running = false;
            if (_metrics_worker.joinable()) _metrics_worker.join();
        }

        void CleanupOldMetrics()
        {
            auto my = CreateConnection();
            if (!my) return;
            std::string sql = "delete from cache_metrics_snapshots where created_at < DATE_SUB(NOW(), INTERVAL 30 DAY)";
            mysql_query(my.get(), sql.c_str());
        }

    private:
        std::atomic<bool> _metrics_running{false};
        std::thread _metrics_worker;
    };

};
