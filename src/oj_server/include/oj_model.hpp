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
#include "../../comm/comm.hpp"
#include "oj_cache.hpp"
#include <mysql/mysql.h>

// model: 主要用来和数据进行交互，对外提供访问数据的接口

namespace ns_model
{
    using namespace ns_log;
    using namespace ns_cache;
    //用户属性
    class Model
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

        using MySqlConn = std::unique_ptr<MYSQL, void(*)(MYSQL*)>;

        MySqlConn CreateConnection()
        {
            MYSQL* my = mysql_init(nullptr);
            if (my == nullptr)
            {
                logger(ns_log::FATAL) << "MySql初始化失败!";
                return MySqlConn(nullptr, mysql_close);
            }

            if (mysql_real_connect(my, host.c_str(), user.c_str(), passwd.c_str(), db.c_str(), port, nullptr, 0) == nullptr)
            {
                logger(ns_log::FATAL) << "MySql连接失败!";
                mysql_close(my);
                return MySqlConn(nullptr, mysql_close);
            }

            return MySqlConn(my, mysql_close);
        }
        //对SQL字符串进行转义，防止SQL注入攻击
        std::string EscapeSqlString(const std::string& input, MYSQL* my)
        {
            std::string escaped;
            escaped.resize(input.size() * 2 + 1);
            unsigned long n = mysql_real_escape_string(my, escaped.data(), input.c_str(), input.size());
            escaped.resize(n);
            return escaped;
        }

        bool IsAllDigits(const std::string& s)
        {
            return !s.empty() && std::all_of(s.begin(), s.end(), [](unsigned char ch) {
                return std::isdigit(ch);
            });
        }

        std::string TrimCopy(const std::string& s)
        {
            size_t begin = 0;
            while (begin < s.size() && std::isspace(static_cast<unsigned char>(s[begin])))
            {
                ++begin;
            }

            size_t end = s.size();
            while (end > begin && std::isspace(static_cast<unsigned char>(s[end - 1])))
            {
                --end;
            }
            return s.substr(begin, end - begin);
        }

        std::string LowerAscii(const std::string& s)
        {
            std::string out = s;
            std::transform(out.begin(), out.end(), out.begin(), [](unsigned char ch) {
                return static_cast<char>(std::tolower(ch));
            });
            return out;
        }

        std::shared_ptr<QuestionQuery> NormalizeQuestionQuery(const std::shared_ptr<QueryStruct>& raw_query)
        {
            auto raw = std::dynamic_pointer_cast<QuestionQuery>(raw_query);
            auto normalized = std::make_shared<QuestionQuery>();
            normalized->id = TrimCopy(raw ? raw->id : "");
            normalized->title = TrimCopy(raw ? raw->title : "");
            normalized->star = LowerAscii(TrimCopy(raw ? raw->star : ""));

            if (!normalized->id.empty() && !IsAllDigits(normalized->id))
            {
                normalized->id.clear();
            }

            return normalized;
        }

        //查询Mysql
        bool QueryMySql(const std::string& sql,std::vector<Question>& questions)
        {
            auto my = CreateConnection();
            if(!my)
                return false;
            if(mysql_query(my.get(), sql.c_str()) != 0)
            {
                logger(ns_log::FATAL)<<"MySql查询错误!";
                return false;
            }
            MYSQL_RES* res = mysql_store_result(my.get());
            if (res == nullptr)
            {
                logger(ns_log::FATAL) << "MySql结果集为空!";
                return false;
            }
            int rows = mysql_num_rows(res);
            //获取题目属性
            for(int i = 0;i<rows;i++)
            {
                MYSQL_ROW row = mysql_fetch_row(res);
                Question q;
                q.number = row[0];
                q.title = row[1];
                q.desc = row[2];
                q.star = row[3];
                q.create_time = row[4];
                q.update_time = row[5];
                q.cpu_limit = atoi(row[6]);
                q.memory_limit = atoi(row[7]);
                questions.push_back(q);
            }
            mysql_free_result(res);
            return true;
        }

        bool QueryCount(const std::string& sql, int* count)
        {
            if (count == nullptr)
                return false;

            auto my = CreateConnection();
            if(!my)
                return false;

            if(mysql_query(my.get(), sql.c_str()) != 0)
            {
                logger(ns_log::FATAL) << "MySql统计查询错误!";
                return false;
            }

            MYSQL_RES* res = mysql_store_result(my.get());
            if (res == nullptr)
            {
                logger(ns_log::FATAL) << "MySql统计结果集为空!";
                return false;
            }

            MYSQL_ROW row = mysql_fetch_row(res);
            if (row == nullptr || row[0] == nullptr)
            {
                mysql_free_result(res);
                return false;
            }

            *count = std::atoi(row[0]);
            mysql_free_result(res);
            return true;
        }

        bool QueryUsers(const std::string& sql, std::vector<User>* users)
        {
            if (users == nullptr)
            {
                return false;
            }

            auto my = CreateConnection();
            if (!my)
                return false;

            if (mysql_query(my.get(), sql.c_str()) != 0)
            {
                logger(ns_log::FATAL) << "MySql用户列表查询错误!";
                return false;
            }

            MYSQL_RES* res = mysql_store_result(my.get());
            if (res == nullptr)
            {
                logger(ns_log::FATAL) << "MySql用户列表结果集为空!";
                return false;
            }

            int rows = mysql_num_rows(res);
            users->clear();
            users->reserve(rows);
            for (int i = 0; i < rows; ++i)
            {
                MYSQL_ROW row = mysql_fetch_row(res);
                if (row == nullptr)
                {
                    continue;
                }
                User item;
                item.uid = row[0] == nullptr ? 0 : std::atoi(row[0]);
                item.name = row[1] == nullptr ? "" : row[1];
                item.email = row[2] == nullptr ? "" : row[2];
                item.create_time = row[3] == nullptr ? "" : row[3];
                item.last_login = row[4] == nullptr ? "" : row[4];
                users->push_back(item);
            }

            mysql_free_result(res);
            return true;
        }

        bool QueryAdminAccount(const std::string& sql, AdminAccount* admin)
        {
            if (admin == nullptr)
            {
                return false;
            }

            auto my = CreateConnection();
            if (!my)
                return false;

            if (mysql_query(my.get(), sql.c_str()) != 0)
            {
                logger(ns_log::FATAL) << "MySql管理员查询错误!";
                return false;
            }

            MYSQL_RES* res = mysql_store_result(my.get());
            if (res == nullptr)
            {
                logger(ns_log::FATAL) << "MySql管理员查询结果集为空!";
                return false;
            }

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

            if (mysql_query(my.get(), sql.c_str()) != 0)
            {
                logger(ns_log::FATAL) << "MySql管理员列表查询错误!";
                return false;
            }

            MYSQL_RES* res = mysql_store_result(my.get());
            if (res == nullptr)
            {
                logger(ns_log::FATAL) << "MySql管理员列表结果集为空!";
                return false;
            }

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

            if (mysql_query(my.get(), sql.c_str()) != 0)
            {
                logger(ns_log::FATAL) << "MySql审计日志查询错误!";
                return false;
            }

            MYSQL_RES* res = mysql_store_result(my.get());
            if (res == nullptr)
            {
                logger(ns_log::FATAL) << "MySql审计日志结果集为空!";
                return false;
            }

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

        //构建题目列表查询的WHERE子句
        std::string BuildQuestionWhereClause(const std::shared_ptr<QueryStruct>& query_hash, MYSQL* my)
        {
            auto q = std::dynamic_pointer_cast<QuestionQuery>(query_hash);
            if (!q) return "";
            //条件列表
            std::vector<std::string> clauses;

            // both 模式映射后会产生 id/title 同值，且 id 为纯数字，此时保持 OR 语义。
            if (!q->id.empty() &&
                !q->title.empty() &&
                q->id == q->title &&
                IsAllDigits(q->id))
            {
                //id title内容相同，并且还都是纯数字，说明用户可能在同时搜索id和title，这时保持OR关系
                std::string safe_title = EscapeSqlString(q->title, my);
                clauses.push_back("(id=" + q->id + " or title like '%" + safe_title + "%')");
            }
            else
            {
                //id和title不同时存在，或者虽然同时存在但内容不同，或者虽然同时存在且内容相同但不是纯数字，这三种情况都保持AND关系
                if (!q->id.empty())
                {
                    if (IsAllDigits(q->id))
                    {
                        //纯数字才允许进入SQL
                        clauses.push_back("id=" + q->id);
                    }
                    else
                    {
                        //否则给一个永远不可能成立的条件，让SQL返回空结果
                        clauses.push_back("1=0");
                    }
                }

                if (!q->title.empty())
                {
                    // title使用模糊匹配
                    std::string safe_title = EscapeSqlString(q->title, my);
                    clauses.push_back("title like '%" + safe_title + "%'");
                }
            }

            //如果difficulty不为空,会把英文难度映射成中文：
            if (!q->star.empty())
            {
                std::string difficulty = q->star;
                if (difficulty == "simple") difficulty = "简单";
                else if (difficulty == "medium") difficulty = "中等";
                else if (difficulty == "hard") difficulty = "困难";

                if (difficulty != "all" && difficulty != "ALL" && difficulty != "全部")
                {
                    std::string safe_star = EscapeSqlString(difficulty, my);
                    clauses.push_back("star='" + safe_star + "'");
                }
            }

            if (clauses.empty())
            {
                return "";
            }

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

            return where.str();
        }
        std::string BuildUserWhereClause(const std::shared_ptr<QueryStruct>& query_hash, MYSQL* my)
        {
            if (!query_hash)
            {
                return "";
            }
            auto uq = std::dynamic_pointer_cast<UserQuery>(query_hash);
            if (!uq)
            {
                return "";
            }

            std::vector<std::string> clauses;
            if (uq->id > 0)
            {
                clauses.push_back("uid=" + std::to_string(uq->id));
            }
            if (!uq->name.empty())
            {
                std::string safe_name = EscapeSqlString(uq->name, my);
                clauses.push_back("name like '%" + safe_name + "%'");
            }
            if (!uq->email.empty())
            {
                std::string safe_email = EscapeSqlString(uq->email, my);
                clauses.push_back("email like '%" + safe_email + "%'");
            }

            if (clauses.empty())
            {
                return "";
            }

            std::ostringstream where;
            where << " where ";
            for (size_t i = 0; i < clauses.size(); ++i)
            {
                if (i != 0)
                {
                    where << " or ";
                }
                where << clauses[i];
            }
            return where.str();
        }

        //查询用户基础信息
        bool QueryUser(const std::string& sql, User* user)
        {
            auto my = CreateConnection();
            if(!my)
                return false;
            if(mysql_query(my.get(), sql.c_str()) != 0)
            {
                logger(ns_log::FATAL)<<"MySql查询错误!";
                return false;
            }
            MYSQL_RES* res = mysql_store_result(my.get());
            if (res == nullptr)
            {
                logger(ns_log::FATAL) << "MySql结果集为空!";
                return false;
            }
            int rows = mysql_num_rows(res);
            if(rows != 1)
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
            user->uid = atoi(row[0]);
            user->name = row[1];
            user->email = row[2];
            user->create_time = row[3];
            user->last_login = row[4];
            user->password_algo = row[5] == nullptr ? "" : row[5];
            mysql_free_result(res);
            return true;
        }

        bool QueryUserPasswordAuth(const std::string& sql, std::string* password_hash, std::string* password_algo)
        {
            if (password_hash == nullptr || password_algo == nullptr)
            {
                return false;
            }

            auto my = CreateConnection();
            if(!my)
                return false;
            if(mysql_query(my.get(), sql.c_str()) != 0)
            {
                logger(ns_log::FATAL) << "MySql查询错误!";
                return false;
            }

            MYSQL_RES* res = mysql_store_result(my.get());
            if (res == nullptr)
            {
                logger(ns_log::FATAL) << "MySql结果集为空!";
                return false;
            }

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

            *password_hash = row[0] == nullptr ? "" : row[0];
            *password_algo = row[1] == nullptr ? "" : row[1];
            mysql_free_result(res);
            return true;
        }

        //执行SQL语句（用于插入、更新等操作）
        bool ExecuteSql(const std::string& sql)
        {
            auto my = CreateConnection();
            if(!my)
                return false;
            if(mysql_query(my.get(), sql.c_str()) != 0)
            {
                logger(ns_log::FATAL) << "MySql执行错误! errno=" << mysql_errno(my.get())
                                      << " error=" << mysql_error(my.get());
                return false;
            }
            return true;
        }

    public:
        std::string SolutionStatusToDbString(SolutionStatus status)
        {
            switch (status)
            {
                case SolutionStatus::pending:
                    return "pending";
                case SolutionStatus::approved:
                    return "approved";
                case SolutionStatus::rejected:
                    return "rejected";
                default:
                    return "approved";
            }
        }

        SolutionStatus DbStringToSolutionStatus(const std::string& s)
        {
            if (s == "pending") return SolutionStatus::pending;
            if (s == "rejected") return SolutionStatus::rejected;
            return SolutionStatus::approved;
        }

        const std::string& AdminAccountsTable() const
        {
            static const std::string table = "admin_accounts";
            return table;
        }

        const std::string& SolutionsTable() const
        {
            static const std::string table = "solutions";
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

        //题库:得到一页内全部的题目
        bool GetQuestionsByPage(std::shared_ptr<Cache::CacheListKey> key,
                             std::vector<Question>& questions,
                             int* total_count,
                             int* total_pages)
        {
            auto begin = std::chrono::steady_clock::now();
            // 参数校验，这一步顺便记录一次列表查询指标，表示这次请求没有成功完成
            if (total_count == nullptr || total_pages == nullptr)
            {
                auto end = std::chrono::steady_clock::now();
                long long cost_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
                RecordListMetrics(false, false, cost_ms);
                return false;
            }
            // 先查缓存:
            //  调用 _cache.GetAllQuestions(...) 尝试从 Redis 读取这一页的数据。
            //  缓存命中时，函数直接返回 true。
            //  这时 questions、total_count、total_pages 都会被缓存结果填好。
            //  同时记录一次“缓存命中”的指标。
            if(_cache.GetQuestionsByPage(key,  questions,total_count, total_pages))
            {
                logger(ns_log::INFO) << "Cache hit for question list page " << key->GetCacheKeyString(&_cache);
                auto end = std::chrono::steady_clock::now();
                long long cost_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
                RecordListMetrics(true, false, cost_ms);
                return true;
            }
            // 缓存没命中就查数据库
            // 先创建 MySQL 连接。
            auto my = CreateConnection();
            if (!my)
            {
                // 如果连接失败，直接返回 false，并记录这次是回源失败。
                auto end = std::chrono::steady_clock::now();
                long long cost_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
                RecordListMetrics(false, true, cost_ms);
                return false;
            }
            // 然后根据查询条件生成 where_clause，拼出统计总数的 SQL。
            std::string where_clause = BuildQuestionWhereClause(key->GetQueryHash(), my.get());
            //先执行 select count(*) ... 得到满足条件的题目总数，写入 total_count。
            std::string count_sql = "select count(*) from " + oj_questions + where_clause;
            if (!QueryCount(count_sql, total_count))
            {
                //如果统计失败，直接返回 false。
                auto end = std::chrono::steady_clock::now();
                long long cost_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
                RecordListMetrics(false, true, cost_ms);
                return false;
            }
            // 如果 total_count <= 0，说明没有任何结果：
            // total_pages 设为 0
            // questions.clear()
            // 仍然把这个空结果写入缓存
            // 返回 true
            if (*total_count <= 0)
            {
                *total_pages = 0;
                questions.clear();
                _cache.SetQuestionsByPage(key, questions,*total_count, *total_pages);
                auto end = std::chrono::steady_clock::now();
                long long cost_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
                RecordListMetrics(false, true, cost_ms);
                return true;
            }
            //计算分页并查当前页数据
            int size = key->GetSize();
            int page = key->GetPage();
            *total_pages = (*total_count + size - 1) / size;
            int safe_page = std::min(page, *total_pages);
            int offset = (safe_page - 1) * size;

            std::ostringstream page_sql;
            page_sql << "select * from " << oj_questions
                     << where_clause
                     << " order by cast(id as unsigned) asc limit " << size << " offset " << offset;

            if (!QueryMySql(page_sql.str(), questions))
            {
                auto end = std::chrono::steady_clock::now();
                long long cost_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
                RecordListMetrics(false, true, cost_ms);
                return false;
            }
            // 回填缓存
            // 数据查出来后，会调用 _cache.SetQuestionsByPage(...) 写回 Redis。
            // 缓存键里包含：
            // 查询条件
            // 页码
            // 每页大小
            // 列表版本号
            // 这样下次同样条件的请求就可以直接命中缓存。
            auto write_key = _cache.BuildListCacheKey(key->GetQueryHash(), safe_page, size, key->GetListVersion(), Cache::CacheKey::PageType::kData, key->GetListType());
            _cache.SetQuestionsByPage(write_key, questions, *total_count, *total_pages);
            auto end = std::chrono::steady_clock::now();
            long long cost_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
            RecordListMetrics(false, true, cost_ms);
            return true;
            // 记录指标并返回
            // 无论是缓存命中还是数据库回源成功，都会调用 RecordListMetrics(...)。
            // 这里的统计不是为了业务逻辑本身，而是为了监控缓存效果和接口耗时。
        }
        //题目:获得单个题目
        bool GetOneQuestion(const std::string& number,Question& q)
        {
            auto begin = std::chrono::steady_clock::now();
            if (!IsAllDigits(number))
            {
                // number参数不合法，直接返回 false，并记录这次请求没有成功完成。
                auto end = std::chrono::steady_clock::now();
                long long cost_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
                RecordDetailMetrics(false, false, cost_ms);
                return false;
            }
            auto detail_key = _cache.BuildDetailCacheKey(number, Cache::CacheKey::PageType::kData);
            // 先查缓存，缓存命中直接返回
            if(_cache.GetDetailedQuestion(detail_key, q))
            {
                logger(ns_log::INFO) << "Cache hit for question " << number;
                auto end = std::chrono::steady_clock::now();
                long long cost_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
                RecordDetailMetrics(true, false, cost_ms);
                return true;
            }
            // 缓存没命中就查数据库，先创建 MySQL 连接
            std::string sql = "select * from ";
            sql += oj_questions;
            sql += " where id=" + number;
            std::vector<Question> v;
            if (!QueryMySql(sql, v))
            {
                //查询数据库失败，直接返回 false，并记录这次是回源失败。
                _cache.SetQuestionNotFound(detail_key, number);
                auto end = std::chrono::steady_clock::now();
                long long cost_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
                RecordDetailMetrics(false, true, cost_ms);
                return false;
            }
            // 数据库查询成功但没有找到这个题目，说明这个题目确实不存在。
            // 为了避免缓存穿透攻击，应该把这个“空结果”也写入缓存（比如写一个特殊的值表示这个题目不存在
            // 这样下次同样的请求就可以直接命中缓存，避免每次都打到数据库上来。
            if(v.size() != 1)
            {
                _cache.SetQuestionNotFound(detail_key, number);
                auto end = std::chrono::steady_clock::now();
                long long cost_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
                RecordDetailMetrics(false, true, cost_ms);
                return false;
            }
            q = v[0];
            _cache.SetDetailedQuestion(detail_key, q);
            auto end = std::chrono::steady_clock::now();
            long long cost_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
            RecordDetailMetrics(false, true, cost_ms);
            return true;
        }

        //题目写接口:新增或修改题目
        bool SaveQuestion(const Question& input)
        {
            if (!IsAllDigits(input.number))
            {
                return false;
            }

            auto my = CreateConnection();
            if (!my)
                return false;

            std::string safe_id = input.number;
            std::string safe_title = EscapeSqlString(input.title, my.get());
            std::string safe_desc = EscapeSqlString(input.desc, my.get());
            std::string safe_star = EscapeSqlString(input.star, my.get());

            std::ostringstream sql;
            sql << "insert into " << oj_questions
                << " (id, title, `desc`, star, cpu_limit, memory_limit, create_time, update_time) values ("
                << safe_id << ", '" << safe_title << "', '" << safe_desc << "', '" << safe_star << "', "
                << input.cpu_limit << ", " << input.memory_limit << ", NOW(), NOW()) "
                << "on duplicate key update "
                << "title=values(title), `desc`=values(`desc`), star=values(star), "
                << "cpu_limit=values(cpu_limit), memory_limit=values(memory_limit), update_time=NOW()";

            if (!ExecuteSql(sql.str()))
            {
                return false;
            }

            Question cached = input;
            auto detail_key = _cache.BuildDetailCacheKey(input.number, Cache::CacheKey::PageType::kData);
            _cache.SetDetailedQuestion(detail_key, cached);
            auto detail_html_key = _cache.BuildDetailCacheKey(input.number, Cache::CacheKey::PageType::kHtml);
            _cache.InvalidatePage(detail_html_key);
            TouchQuestionListVersion();
            return true;
        }

        bool DeleteQuestion(const std::string& number)
        {
            if (!IsAllDigits(number))
            {
                return false;
            }

            std::string sql = "delete from " + oj_questions + " where id=" + number;
            if (!ExecuteSql(sql))
            {
                return false;
            }

            auto detail_key = _cache.BuildDetailCacheKey(number, Cache::CacheKey::PageType::kData);
            _cache.SetQuestionNotFound(detail_key, number);
            auto detail_html_key = _cache.BuildDetailCacheKey(number, Cache::CacheKey::PageType::kHtml);
            _cache.InvalidatePage(detail_html_key);
            TouchQuestionListVersion();
            return true;
        }

        bool CreateSolution(const Solution& input, unsigned long long* solution_id = nullptr)
        {
            if (input.question_id.empty() || input.user_id <= 0)
            {
                return false;
            }

            std::string trimmed_title = TrimCopy(input.title);
            std::string trimmed_content = TrimCopy(input.content_md);
            if (trimmed_title.empty() || trimmed_content.empty())
            {
                return false;
            }

            auto my = CreateConnection();
            if (!my)
            {
                return false;
            }

            std::string safe_question_id = EscapeSqlString(input.question_id, my.get());
            std::string safe_title = EscapeSqlString(trimmed_title, my.get());
            std::string safe_content = EscapeSqlString(trimmed_content, my.get());
            std::string safe_status = EscapeSqlString(SolutionStatusToDbString(input.status), my.get());

            std::ostringstream sql;
            sql << "insert into " << SolutionsTable()
                << " (question_id, user_id, title, content_md, like_count, favorite_count, comment_count, status, created_at, updated_at) values ('"
                << safe_question_id << "', "
                << input.user_id << ", '"
                << safe_title << "', '"
                << safe_content << "', 0, 0, 0, '"
                << safe_status << "', NOW(), NOW())";

            if (mysql_query(my.get(), sql.str().c_str()) != 0)
            {
                logger(ns_log::FATAL) << "MySql执行错误! errno=" << mysql_errno(my.get())
                                      << " error=" << mysql_error(my.get());
                return false;
            }

            if (solution_id != nullptr)
            {
                *solution_id = static_cast<unsigned long long>(mysql_insert_id(my.get()));
            }
            return true;
        }

        const std::string& SolutionActionsTable() const
        {
            static const std::string table = "solution_actions";
            return table;
        }

        bool GetSolutionsByPage(const std::string& question_id,
                                const std::string& status_filter,
                                const std::string& sort_order,
                                int page,
                                int size,
                                std::vector<Solution>* solutions,
                                int* total_count,
                                int* total_pages)
        {
            if (solutions == nullptr || total_count == nullptr || total_pages == nullptr)
            {
                return false;
            }

            auto my = CreateConnection();
            if (!my)
            {
                return false;
            }

            std::string safe_qid = EscapeSqlString(question_id, my.get());
            std::string safe_status = EscapeSqlString(status_filter, my.get());

            std::string where_clause = " where question_id='" + safe_qid + "'";
            if (!safe_status.empty())
            {
                where_clause += " and status='" + safe_status + "'";
            }

            std::string count_sql = "select count(*) from " + SolutionsTable() + where_clause;
            if (!QueryCount(count_sql, total_count))
            {
                return false;
            }

            if (*total_count <= 0)
            {
                *total_pages = 0;
                solutions->clear();
                return true;
            }

            int safe_page = std::max(1, page);
            int safe_size = std::max(1, std::min(size, 50));
            *total_pages = (*total_count + safe_size - 1) / safe_size;
            int safe_page_clamped = std::min(safe_page, *total_pages);
            int offset = (safe_page_clamped - 1) * safe_size;

            std::string order_clause = " order by id asc";
            if (sort_order == "hot")
            {
                order_clause = " order by like_count desc, id asc";
            }

            std::ostringstream page_sql;
            page_sql << "select id, question_id, user_id, title, like_count, favorite_count, comment_count, status, created_at, updated_at from "
                      << SolutionsTable()
                      << where_clause
                      << order_clause
                      << " limit " << safe_size << " offset " << offset;

            MYSQL_RES* res = nullptr;
            if (mysql_query(my.get(), page_sql.str().c_str()) != 0)
            {
                logger(ns_log::FATAL) << "MySql题解列表查询错误!";
                return false;
            }

            res = mysql_store_result(my.get());
            if (res == nullptr)
            {
                logger(ns_log::FATAL) << "MySql题解列表结果集为空!";
                return false;
            }

            int rows = mysql_num_rows(res);
            solutions->clear();
            solutions->reserve(rows);
            for (int i = 0; i < rows; ++i)
            {
                MYSQL_ROW row = mysql_fetch_row(res);
                if (row == nullptr) continue;

                Solution s;
                s.id = (row[0] != nullptr) ? std::stoull(row[0]) : 0;
                s.question_id = (row[1] != nullptr) ? row[1] : "";
                s.user_id = (row[2] != nullptr) ? std::atoi(row[2]) : 0;
                s.title = (row[3] != nullptr) ? row[3] : "";
                s.like_count = (row[4] != nullptr) ? static_cast<unsigned int>(std::atoi(row[4])) : 0;
                s.favorite_count = (row[5] != nullptr) ? static_cast<unsigned int>(std::atoi(row[5])) : 0;
                s.comment_count = (row[6] != nullptr) ? static_cast<unsigned int>(std::atoi(row[6])) : 0;
                s.status = (row[7] != nullptr) ? DbStringToSolutionStatus(row[7]) : SolutionStatus::approved;
                s.created_at = (row[8] != nullptr) ? row[8] : "";
                s.updated_at = (row[9] != nullptr) ? row[9] : "";
                solutions->push_back(s);
            }

            mysql_free_result(res);
            return true;
        }

        bool GetSolutionById(long long solution_id, Solution* solution)
        {
            if (solution == nullptr)
            {
                return false;
            }

            auto my = CreateConnection();
            if (!my)
            {
                return false;
            }

            std::ostringstream sql;
            sql << "select id, question_id, user_id, title, content_md, like_count, favorite_count, comment_count, status, created_at, updated_at from "
                << SolutionsTable() << " where id=" << solution_id;

            if (mysql_query(my.get(), sql.str().c_str()) != 0)
            {
                logger(ns_log::FATAL) << "MySql题解详情查询错误!";
                return false;
            }

            MYSQL_RES* res = mysql_store_result(my.get());
            if (res == nullptr)
            {
                logger(ns_log::FATAL) << "MySql题解详情结果集为空!";
                return false;
            }

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

            solution->id = (row[0] != nullptr) ? std::stoull(row[0]) : 0;
            solution->question_id = (row[1] != nullptr) ? row[1] : "";
            solution->user_id = (row[2] != nullptr) ? std::atoi(row[2]) : 0;
            solution->title = (row[3] != nullptr) ? row[3] : "";
            solution->content_md = (row[4] != nullptr) ? row[4] : "";
            solution->like_count = (row[5] != nullptr) ? static_cast<unsigned int>(std::atoi(row[5])) : 0;
            solution->favorite_count = (row[6] != nullptr) ? static_cast<unsigned int>(std::atoi(row[6])) : 0;
            solution->comment_count = (row[7] != nullptr) ? static_cast<unsigned int>(std::atoi(row[7])) : 0;
            solution->status = (row[8] != nullptr) ? DbStringToSolutionStatus(row[8]) : SolutionStatus::approved;
            solution->created_at = (row[9] != nullptr) ? row[9] : "";
            solution->updated_at = (row[10] != nullptr) ? row[10] : "";

            mysql_free_result(res);
            return true;
        }

        bool ToggleSolutionAction(long long solution_id, int user_id, const std::string& action_type,
                                   bool* now_active, unsigned int* new_count)
        {
            if (now_active == nullptr || new_count == nullptr)
            {
                return false;
            }

            auto my = CreateConnection();
            if (!my)
            {
                return false;
            }

            std::string safe_action = EscapeSqlString(action_type, my.get());

            std::string count_column;
            if (action_type == "like")
            {
                count_column = "like_count";
            }
            else if (action_type == "favorite")
            {
                count_column = "favorite_count";
            }
            else
            {
                return false;
            }

            std::ostringstream check_sql;
            check_sql << "select id from " << SolutionActionsTable()
                      << " where solution_id=" << solution_id
                      << " and user_id=" << user_id
                      << " and action_type='" << safe_action << "'";

            if (mysql_query(my.get(), check_sql.str().c_str()) != 0)
            {
                logger(ns_log::FATAL) << "MySql查询交互记录错误!";
                return false;
            }

            MYSQL_RES* res = mysql_store_result(my.get());
            if (res == nullptr)
            {
                logger(ns_log::FATAL) << "MySql交互记录结果集为空!";
                return false;
            }

            int exist_rows = mysql_num_rows(res);
            mysql_free_result(res);

            if (exist_rows > 0)
            {
                std::ostringstream del_sql;
                del_sql << "delete from " << SolutionActionsTable()
                        << " where solution_id=" << solution_id
                        << " and user_id=" << user_id
                        << " and action_type='" << safe_action << "'";

                if (mysql_query(my.get(), del_sql.str().c_str()) != 0)
                {
                    logger(ns_log::FATAL) << "MySql删除交互记录错误!";
                    return false;
                }

                std::ostringstream dec_sql;
                dec_sql << "update " << SolutionsTable()
                        << " set " << count_column << "=" << count_column << "-1"
                        << " where id=" << solution_id;

                if (mysql_query(my.get(), dec_sql.str().c_str()) != 0)
                {
                    logger(ns_log::FATAL) << "MySql更新计数错误!";
                    return false;
                }

                *now_active = false;
            }
            else
            {
                std::ostringstream ins_sql;
                ins_sql << "insert into " << SolutionActionsTable()
                        << " (solution_id, user_id, action_type, created_at) values ("
                        << solution_id << ", " << user_id << ", '" << safe_action << "', NOW())";

                if (mysql_query(my.get(), ins_sql.str().c_str()) != 0)
                {
                    logger(ns_log::FATAL) << "MySql插入交互记录错误!";
                    return false;
                }

                std::ostringstream inc_sql;
                inc_sql << "update " << SolutionsTable()
                        << " set " << count_column << "=" << count_column << "+1"
                        << " where id=" << solution_id;

                if (mysql_query(my.get(), inc_sql.str().c_str()) != 0)
                {
                    logger(ns_log::FATAL) << "MySql更新计数错误!";
                    return false;
                }

                *now_active = true;
            }

            std::ostringstream cnt_sql;
            cnt_sql << "select " << count_column << " from " << SolutionsTable() << " where id=" << solution_id;

            if (mysql_query(my.get(), cnt_sql.str().c_str()) != 0)
            {
                logger(ns_log::FATAL) << "MySql查询计数错误!";
                return false;
            }

            MYSQL_RES* cnt_res = mysql_store_result(my.get());
            if (cnt_res == nullptr)
            {
                logger(ns_log::FATAL) << "MySql计数结果集为空!";
                return false;
            }

            MYSQL_ROW cnt_row = mysql_fetch_row(cnt_res);
            if (cnt_row == nullptr || cnt_row[0] == nullptr)
            {
                mysql_free_result(cnt_res);
                return false;
            }

            *new_count = static_cast<unsigned int>(std::atoi(cnt_row[0]));
            mysql_free_result(cnt_res);
            return true;
        }

        bool GetUserActionsForSolutions(int user_id, const std::vector<long long>& solution_ids,
                                        std::map<long long, std::map<std::string, bool>>& actions)
        {
            if (solution_ids.empty())
            {
                return true;
            }

            auto my = CreateConnection();
            if (!my)
            {
                return false;
            }

            std::ostringstream ids_stream;
            for (size_t i = 0; i < solution_ids.size(); ++i)
            {
                if (i > 0) ids_stream << ",";
                ids_stream << solution_ids[i];
            }

            std::ostringstream sql;
            sql << "select solution_id, action_type from " << SolutionActionsTable()
                << " where user_id=" << user_id
                << " and solution_id in (" << ids_stream.str() << ")";

            if (mysql_query(my.get(), sql.str().c_str()) != 0)
            {
                logger(ns_log::FATAL) << "MySql查询用户交互记录错误!";
                return false;
            }

            MYSQL_RES* res = mysql_store_result(my.get());
            if (res == nullptr)
            {
                logger(ns_log::FATAL) << "MySql用户交互记录结果集为空!";
                return false;
            }

            for (int i = 0; i < mysql_num_rows(res); ++i)
            {
                MYSQL_ROW row = mysql_fetch_row(res);
                if (row == nullptr) continue;
                long long sid = std::stoll(row[0]);
                std::string atype = row[1] ? row[1] : "";
                actions[sid][atype] = true;
            }

            mysql_free_result(res);
            return true;
        }

        //用户:检查用户是否存在
        bool CheckUser(const std::string& email)
        {
            auto my = CreateConnection();
            if(!my)
                return false;

            std::string safe_email = EscapeSqlString(email, my.get());
            std::string sql = "select * from " + oj_users + " where email='" + safe_email + "'";

            if(mysql_query(my.get(), sql.c_str()) != 0)
            {
                logger(ns_log::FATAL)<<"MySql查询错误!";
                return false;
            }
            MYSQL_RES* res = mysql_store_result(my.get());
            if (res == nullptr)
            {
                logger(ns_log::FATAL) << "MySql结果集为空!";
                return false;
            }
            int rows = mysql_num_rows(res);
            mysql_free_result(res);
            return rows > 0;
        }

        //用户:创建新用户
        bool CreateUser(const std::string& name, const std::string& email)
        {
            auto my = CreateConnection();
            if(!my)
                return false;

            std::string safe_name = EscapeSqlString(name, my.get());
            std::string safe_email = EscapeSqlString(email, my.get());
            std::string sql = "insert into " + oj_users +
                              " (name, password_hash, email, password_algo) values ('" +
                              safe_name + "', '', '" + safe_email + "', 'none')";
            return ExecuteSql(sql);
        }

        //用户:获取用户信息
        bool GetUser(const std::string& email, User* user)
        {
            auto my = CreateConnection();
            if(!my)
                return false;

            std::string safe_email = EscapeSqlString(email, my.get());
            std::string sql = "select uid,name,email,create_time,last_login,password_algo from " + oj_users + " where email='" + safe_email + "'";
            return QueryUser(sql, user);
        }

        //用户:更新最后登录时间
        bool UpdateLastLogin(const std::string& email)
        {
            auto my = CreateConnection();
            if(!my)
                return false;

            std::string safe_email = EscapeSqlString(email, my.get());
            std::string sql = "update " + oj_users + " set last_login=NOW() where email='" + safe_email + "'";
            return ExecuteSql(sql);
        }

        //用户:通过ID获取用户信息
        bool GetUserById(int uid, User* user)
        {
            std::string sql = "select uid,name,email,create_time,last_login,password_algo from " + oj_users + " where uid=" + std::to_string(uid);
            return QueryUser(sql, user);
        }

        bool GetUserCount(int* count)
        {
            if(count == nullptr)
            {
                return false;
            }
            std::string key = "user_counts";
            std::string value;
            if(_cache.GetStringByAnyKey(key, &value))
            {
                *count = std::atoi(value.c_str());
                logger(ns_log::INFO) << "Cache hit for user count";
                return true;
            }
            std::string sql = "select count(*) from " + oj_users;
            if(!QueryCount(sql, count))
            {
                return false;
             }
             _cache.SetStringByAnyKey(key, std::to_string(*count), _cache.BuildJitteredTtl(6 * 60 * 60, 30 * 60));
             return true;
        }

        bool GetQuestionCount(int* count)
        {
            if(count == nullptr)
            {
                return false;
            }
            std::string key = "question_counts";
            std::string value;
            if(_cache.GetStringByAnyKey(key, &value))
            {
                *count = std::atoi(value.c_str());
                logger(ns_log::INFO) << "Cache hit for question count";
                return true;
            }
            std::string sql = "select count(*) from " + oj_questions;
            if(!QueryCount(sql, count))
            {
                return false;
             }
             _cache.SetStringByAnyKey(key, std::to_string(*count), _cache.BuildJitteredTtl(6 * 60 * 60, 30 * 60));
             return true;
        }
        bool GetUsersPaged(std::shared_ptr<Cache::CacheListKey> key,std::vector<User>* users, int* total_count,int * total_pages)
        {
            auto begin = std::chrono::steady_clock::now();
            if (users == nullptr || total_count == nullptr || total_pages == nullptr)
            {
                auto end = std::chrono::steady_clock::now();
                long long cost_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
                RecordListMetrics(false, false, cost_ms);
                return false;
            }

            if (_cache.GetUsersByPage(key, *users, total_count, total_pages))
            {
                logger(ns_log::INFO) << "Cache hit for user list page " << key->GetCacheKeyString(&_cache);
                auto end = std::chrono::steady_clock::now();
                long long cost_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
                RecordListMetrics(true, false, cost_ms);
                return true;
            }

            auto my = CreateConnection();
            if (!my)
            {
                auto end = std::chrono::steady_clock::now();
                long long cost_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
                RecordListMetrics(false, true, cost_ms);
                return false;
            }

            std::string where_clause = BuildUserWhereClause(key->GetQueryHash(), my.get());

            std::string count_sql = "select count(*) from " + oj_users + where_clause;
            if (!QueryCount(count_sql, total_count))
            {
                auto end = std::chrono::steady_clock::now();
                long long cost_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
                RecordListMetrics(false, true, cost_ms);
                return false;
            }

            if (*total_count <= 0)
            {
                *total_pages = 0;
                users->clear();
                _cache.SetUsersByPage(key, *users, *total_count, *total_pages);
                auto end = std::chrono::steady_clock::now();
                long long cost_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
                RecordListMetrics(false, true, cost_ms);
                return true;
            }

            int size = key->GetSize();
            int page = key->GetPage();
            *total_pages = (*total_count + size - 1) / size;
            int safe_page = std::min(page, *total_pages);
            int offset = (safe_page - 1) * size;

            std::ostringstream page_sql;
            page_sql << "select uid,name,email,create_time,last_login from " << oj_users
                     << where_clause
                     << " order by uid desc limit " << size << " offset " << offset;

            if (!QueryUsers(page_sql.str(), users))
            {
                auto end = std::chrono::steady_clock::now();
                long long cost_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
                RecordListMetrics(false, true, cost_ms);
                return false;
            }

            auto write_key = _cache.BuildListCacheKey(key->GetQueryHash(), safe_page, size, key->GetListVersion(), Cache::CacheKey::PageType::kData, key->GetListType());
            _cache.SetUsersByPage(write_key, *users, *total_count, *total_pages);
            auto end = std::chrono::steady_clock::now();
            long long cost_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
            RecordListMetrics(false, true, cost_ms);
            return true;
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

        bool SetUserPassword(const std::string& email, const std::string& password_hash, const std::string& password_algo)
        {
            auto my = CreateConnection();
            if(!my)
                return false;

            std::string safe_email = EscapeSqlString(email, my.get());
            std::string safe_hash = EscapeSqlString(password_hash, my.get());
            std::string safe_algo = EscapeSqlString(password_algo, my.get());
            std::string sql = "update " + oj_users +
                              " set password_hash='" + safe_hash +
                              "', password_algo='" + safe_algo +
                              "', password_update_at=NOW() where email='" + safe_email + "'";
            return ExecuteSql(sql);
        }

        bool GetUserPasswordAuth(const std::string& email, std::string* password_hash, std::string* password_algo)
        {
            auto my = CreateConnection();
            if(!my)
                return false;

            std::string safe_email = EscapeSqlString(email, my.get());
            std::string sql = "select password_hash,password_algo from " + oj_users + " where email='" + safe_email + "'";
            return QueryUserPasswordAuth(sql, password_hash, password_algo);
        }

        bool UpdateUserEmail(const std::string& old_email, const std::string& new_email)
        {
            auto my = CreateConnection();
            if(!my)
                return false;

            std::string safe_old_email = EscapeSqlString(old_email, my.get());
            std::string safe_new_email = EscapeSqlString(new_email, my.get());
            std::string sql = "update " + oj_users +
                              " set email='" + safe_new_email +
                              "' where email='" + safe_old_email + "'";
            return ExecuteSql(sql);
        }

        bool DeleteUserByEmail(const std::string& email)
        {
            auto my = CreateConnection();
            if(!my)
                return false;

            std::string safe_email = EscapeSqlString(email, my.get());
            std::string sql = "delete from " + oj_users + " where email='" + safe_email + "'";
            return ExecuteSql(sql);
        }

        bool GetUserByName(const std::string& name, User* user)
        {
            auto my = CreateConnection();
            if(!my)
                return false;

            std::string safe_name = EscapeSqlString(name, my.get());
            std::string sql = "select uid,name,email,create_time,last_login,password_algo from " + oj_users + " where name='" + safe_name + "'";
            return QueryUser(sql, user);
        }

        bool RecordUserSubmit(int user_id, const std::string& question_id, const std::string& result_json, bool is_pass)
        {
            auto my = CreateConnection();
            if (!my)
                return false;

            std::string safe_qid = EscapeSqlString(question_id, my.get());
            std::string safe_json = EscapeSqlString(result_json, my.get());

            std::ostringstream sql;
            sql << "insert into user_submits (user_id, question_id, result_json, is_pass, action_time) values ("
                << user_id << ", '" << safe_qid << "', '" << safe_json << "', " << (is_pass ? 1 : 0) << ", NOW())";
            return ExecuteSql(sql.str());
        }

        bool HasUserPassedQuestion(int user_id, const std::string& question_id)
        {
            auto my = CreateConnection();
            if (!my)
                return false;

            std::string safe_qid = EscapeSqlString(question_id, my.get());
            std::ostringstream sql;
            sql << "select count(*) from user_submits where user_id=" << user_id
                << " and question_id='" << safe_qid << "' and is_pass=1";

            int count = 0;
            if (!QueryCount(sql.str(), &count))
                return false;
            return count > 0;
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

        // 题目写路径完成后调用该接口，统一触发列表版本递增。
        std::string TouchQuestionListVersion()
        {   
            std::string version = _cache.BumpListVersion(ListType::Questions);
            logger(ns_log::INFO) << "Question list version bumped to " << version;
            return version;
        }
        std::string GetQuestionsListVersion()
        {
            return _cache.GetListVersion(ListType::Questions);
        }
        std::shared_ptr<Cache::CacheListKey> BuildListCacheKey(std::shared_ptr<QueryStruct>& query_hash, int page, int size, const std::string& list_version, Cache::CacheKey::PageType page_type)
        {
            auto normalized_query = NormalizeQuestionQuery(query_hash);
            return _cache.BuildListCacheKey(normalized_query, page, size, list_version, page_type, ListType::Questions);
        }
        std::shared_ptr<Cache::CacheDetailKey> BuildDetailCacheKey(const std::string& number, Cache::CacheKey::PageType page_type)
        {
            return _cache.BuildDetailCacheKey(number, page_type);
        }
        std::shared_ptr<Cache::CacheStaticKey> BuildHtmlCacheKey(const std::string& page_name, Cache::CacheKey::PageType page_type)
        {
            return _cache.BuildStaticCacheKey(page_name, page_type);
        }
    private:
        Cache _cache;
    };

};