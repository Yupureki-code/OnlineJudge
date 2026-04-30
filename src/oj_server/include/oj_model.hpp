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

    public:
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
    private:
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
                logger(ns_log::FATAL)<<"MySql查询错误: "<<mysql_error(my.get());
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
                item.avatar_path = row[3] == nullptr ? "" : row[3];
                item.create_time = row[4] == nullptr ? "" : row[4];
                item.last_login = row[5] == nullptr ? "" : row[5];
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
                logger(ns_log::FATAL)<<"MySql查询错误: "<<mysql_error(my.get());
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
            user->avatar_path = row[3] == nullptr ? "" : row[3];
            user->create_time = row[4];
            user->last_login = row[5];
            user->password_algo = row[6] == nullptr ? "" : row[6];
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
                logger(ns_log::FATAL) << "MySql查询错误: " << mysql_error(my.get());
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

            // Invalidate solution list cache for this question (common combinations)
            auto list_key_latest = _cache.BuildSolutionCacheKey(
                input.question_id, 1, 10, "1",
                Cache::CacheKey::PageType::kList,
                SolutionStatus::approved, SolutionSort::latest);
            _cache.DeleteStringByAnyKey(list_key_latest->GetCacheKeyString(&_cache));

            auto list_key_hot = _cache.BuildSolutionCacheKey(
                input.question_id, 1, 10, "1",
                Cache::CacheKey::PageType::kList,
                SolutionStatus::approved, SolutionSort::hot);
            _cache.DeleteStringByAnyKey(list_key_hot->GetCacheKeyString(&_cache));

            logger(ns_log::INFO) << "Invalidated solution list cache for question " << input.question_id;

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

            // Build cache key for solution list
            SolutionStatus status = SolutionStatus::approved;
            if (status_filter == "pending")
                status = SolutionStatus::pending;
            else if (status_filter == "rejected")
                status = SolutionStatus::rejected;

            SolutionSort sort = SolutionSort::latest;
            if (sort_order == "hot")
                sort = SolutionSort::hot;

            auto cache_key = _cache.BuildSolutionCacheKey(question_id, page, size, "1",
                                                          Cache::CacheKey::PageType::kList,
                                                          status, sort);

            // Try to read from cache
            std::string cached_json;
            if (_cache.GetStringByAnyKey(cache_key->GetCacheKeyString(&_cache), &cached_json))
            {
                // Cache hit: parse JSON and populate results
                Json::CharReaderBuilder builder;
                Json::Value json_value;
                std::istringstream ss(cached_json);
                if (Json::parseFromStream(builder, ss, &json_value, nullptr))
                {
                    if (json_value.isMember("total"))
                        *total_count = json_value["total"].asInt();
                    if (json_value.isMember("total_pages"))
                        *total_pages = json_value["total_pages"].asInt();

                    if (json_value.isMember("solutions") && json_value["solutions"].isArray())
                    {
                        for (const auto& item : json_value["solutions"])
                        {
                            Solution s;
                            s.id = item["id"].asUInt64();
                            s.question_id = item["question_id"].asString();
                            s.user_id = item["user_id"].asInt();
                            s.title = item["title"].asString();
                            s.content_md = item["content_md"].asString();
                            s.like_count = item["like_count"].asUInt();
                            s.favorite_count = item["favorite_count"].asUInt();
                            s.comment_count = item["comment_count"].asUInt();
                            s.status = DbStringToSolutionStatus(item["status"].asString());
                            s.created_at = item["created_at"].asString();
                            s.updated_at = item["updated_at"].asString();
                            solutions->push_back(s);
                        }
                    }
                    logger(ns_log::INFO) << "Cache hit for solution list " << cache_key->GetCacheKeyString(&_cache);
                    return true;
                }
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
            page_sql << "select id, question_id, user_id, title, content_md, like_count, favorite_count, comment_count, status, created_at, updated_at from "
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
                try
                {
                    s.id = (row[0] != nullptr) ? std::stoull(row[0]) : 0;
                }
                catch (const std::exception&) { s.id = 0; }
                s.question_id = (row[1] != nullptr) ? row[1] : "";
                s.user_id = (row[2] != nullptr) ? std::atoi(row[2]) : 0;
                s.title = (row[3] != nullptr) ? row[3] : "";
                s.content_md = (row[4] != nullptr) ? row[4] : "";
                s.like_count = (row[5] != nullptr) ? static_cast<unsigned int>(std::atoi(row[5])) : 0;
                s.favorite_count = (row[6] != nullptr) ? static_cast<unsigned int>(std::atoi(row[6])) : 0;
                s.comment_count = (row[7] != nullptr) ? static_cast<unsigned int>(std::atoi(row[7])) : 0;
                s.status = (row[8] != nullptr) ? DbStringToSolutionStatus(row[8]) : SolutionStatus::approved;
                s.created_at = (row[9] != nullptr) ? row[9] : "";
                s.updated_at = (row[10] != nullptr) ? row[10] : "";
                solutions->push_back(s);
            }

            mysql_free_result(res);

            // Write to cache
            Json::Value root(Json::objectValue);
            root["total"] = *total_count;
            root["total_pages"] = *total_pages;
            Json::Value array_value(Json::arrayValue);
            for (const auto& s : *solutions)
            {
                Json::Value item;
                item["id"] = static_cast<Json::UInt64>(s.id);
                item["question_id"] = s.question_id;
                item["user_id"] = s.user_id;
                item["title"] = s.title;
                item["content_md"] = s.content_md;
                item["like_count"] = s.like_count;
                item["favorite_count"] = s.favorite_count;
                item["comment_count"] = s.comment_count;
                item["status"] = SolutionStatusToDbString(s.status);
                item["created_at"] = s.created_at;
                item["updated_at"] = s.updated_at;
                array_value.append(item);
            }
            root["solutions"] = array_value;
            Json::FastWriter writer;
            std::string json_str = writer.write(root);
            _cache.SetStringByAnyKey(cache_key->GetCacheKeyString(&_cache), json_str,
                                     _cache.BuildJitteredTtl(600, 120));
            logger(ns_log::INFO) << "Cache miss for solution list, written to cache " << cache_key->GetCacheKeyString(&_cache);

            return true;
        }

        bool GetSolutionById(long long solution_id, Solution* solution)
        {
            if (solution == nullptr)
            {
                return false;
            }

            // Build cache key for solution detail
            auto cache_key = _cache.BuildSolutionDetailCacheKey(solution_id);

            // Try to read from cache
            std::string cached_json;
            if (_cache.GetStringByAnyKey(cache_key->GetCacheKeyString(&_cache), &cached_json))
            {
                Json::CharReaderBuilder builder;
                Json::Value json_value;
                std::istringstream ss(cached_json);
                if (Json::parseFromStream(builder, ss, &json_value, nullptr))
                {
                    solution->id = json_value["id"].asUInt64();
                    solution->question_id = json_value["question_id"].asString();
                    solution->user_id = json_value["user_id"].asInt();
                    solution->title = json_value["title"].asString();
                    solution->content_md = json_value["content_md"].asString();
                    solution->like_count = json_value["like_count"].asUInt();
                    solution->favorite_count = json_value["favorite_count"].asUInt();
                    solution->comment_count = json_value["comment_count"].asUInt();
                    solution->status = DbStringToSolutionStatus(json_value["status"].asString());
                    solution->created_at = json_value["created_at"].asString();
                    solution->updated_at = json_value["updated_at"].asString();
                    logger(ns_log::INFO) << "Cache hit for solution detail " << cache_key->GetCacheKeyString(&_cache);
                    return true;
                }
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

            solution->id = 0;
            try
            {
                if (row[0] != nullptr) solution->id = std::stoull(row[0]);
            }
            catch (const std::exception&) { solution->id = 0; }
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

            // Write to cache
            Json::Value json_value;
            json_value["id"] = static_cast<Json::UInt64>(solution->id);
            json_value["question_id"] = solution->question_id;
            json_value["user_id"] = solution->user_id;
            json_value["title"] = solution->title;
            json_value["content_md"] = solution->content_md;
            json_value["like_count"] = solution->like_count;
            json_value["favorite_count"] = solution->favorite_count;
            json_value["comment_count"] = solution->comment_count;
            json_value["status"] = SolutionStatusToDbString(solution->status);
            json_value["created_at"] = solution->created_at;
            json_value["updated_at"] = solution->updated_at;
            Json::FastWriter writer;
            std::string json_str = writer.write(json_value);
            _cache.SetStringByAnyKey(cache_key->GetCacheKeyString(&_cache), json_str,
                                     _cache.BuildJitteredTtl(600, 120));
            logger(ns_log::INFO) << "Cache miss for solution detail, written to cache " << cache_key->GetCacheKeyString(&_cache);

            return true;
        }

        // Comments: get comments by a solution with pagination (top-level only by default)
        bool GetCommentsBySolutionId(unsigned long long solution_id, int page, int size,
                                     std::vector<Comment>* comments, int* total_count)
        {
            if (comments == nullptr || total_count == nullptr)
            {
                return false;
            }

            // Build cache key: comment:list:sid:{solution_id}:page:{page}:size:{size}
            std::string cache_key = "comment:list:sid:" + std::to_string(solution_id)
                                  + ":page:" + std::to_string(page)
                                  + ":size:" + std::to_string(size);

            // Try to read from cache
            std::string cached_json;
            if (_cache.GetStringByAnyKey(cache_key, &cached_json))
            {
                // Cache hit: parse JSON and populate results
                Json::CharReaderBuilder builder;
                Json::Value json_value;
                std::istringstream ss(cached_json);
                if (Json::parseFromStream(builder, ss, &json_value, nullptr))
                {
                    if (json_value.isMember("total"))
                        *total_count = json_value["total"].asInt();

                    if (json_value.isMember("comments") && json_value["comments"].isArray())
                    {
                        for (const auto& item : json_value["comments"])
                        {
                            Comment c;
                            c.id = item["id"].asUInt64();
                            c.solution_id = item["solution_id"].asUInt64();
                            c.user_id = item["user_id"].asInt();
                            c.content = item["content"].asString();
                            c.is_edited = item["is_edited"].asBool();
                            c.parent_id = item["parent_id"].asUInt64();
                            c.reply_to_user_id = item["reply_to_user_id"].asInt();
                            c.like_count = item["like_count"].asUInt();
                            c.favorite_count = item["favorite_count"].asUInt();
                            c.created_at = item["created_at"].asString();
                            c.updated_at = item["updated_at"].asString();
                            c.reply_to_user_name = item["reply_to_user_name"].asString();
                            comments->push_back(c);
                        }
                    }
                    logger(ns_log::INFO) << "Cache hit for comment list " << cache_key;
                    return true;
                }
            }

            auto my = CreateConnection();
            if (!my)
            {
                return false;
            }

            // paging parameters
            int safe_page = std::max(1, page);
            int safe_size = std::max(1, size);
            int offset = (safe_page - 1) * safe_size;

            // top-level comments only (parent_id NULL or 0)
            std::string count_sql = "select count(*) from solution_comments where solution_id=" + std::to_string(solution_id) + " and (parent_id IS NULL OR parent_id = 0)";
            if (!QueryCount(count_sql, total_count))
            {
                return false;
            }
            if (*total_count <= 0)
            {
                comments->clear();
                return true;
            }

            std::ostringstream sql;
            // include counts for replies and the reply_to information
            sql << "select c.id, c.solution_id, c.user_id, c.content, c.is_edited, c.parent_id, c.reply_to_user_id, c.like_count, c.favorite_count, c.created_at, c.updated_at, u.name AS author_name, ru.name AS reply_to_user_name "
                << "from solution_comments c LEFT JOIN users u ON c.user_id = u.uid LEFT JOIN users ru ON c.reply_to_user_id = ru.uid "
                << "where c.solution_id=" << solution_id
                << " and (c.parent_id IS NULL OR c.parent_id = 0)"
                << " ORDER BY c.created_at ASC LIMIT " << safe_size << " OFFSET " << offset;

            if (mysql_query(my.get(), sql.str().c_str()) != 0)
            {
                logger(ns_log::FATAL) << "MySql查询题解评论错误!";
                return false;
            }

            MYSQL_RES* res = mysql_store_result(my.get());
            if (res == nullptr)
            {
                logger(ns_log::FATAL) << "MySql题解评论结果集为空!";
                return false;
            }

            int rows = mysql_num_rows(res);
            comments->clear();
            comments->reserve(rows);
            for (int i = 0; i < rows; ++i)
            {
                MYSQL_ROW row = mysql_fetch_row(res);
                if (row == nullptr)
                {
                    continue;
                }
                Comment c;
                try
                {
                    c.id = (row[0] != nullptr) ? std::stoull(row[0]) : 0;
                    c.solution_id = (row[1] != nullptr) ? std::stoull(row[1]) : 0;
                }
                catch (const std::exception&) { c.id = 0; c.solution_id = 0; }
                c.user_id = (row[2] != nullptr) ? std::atoi(row[2]) : 0;
                c.content = (row[3] != nullptr) ? row[3] : "";
                c.is_edited = (row[4] != nullptr) ? (std::atoi(row[4]) != 0) : false;
                c.parent_id = (row[5] != nullptr) ? static_cast<unsigned long long>(std::stoull(row[5])) : 0;
                c.reply_to_user_id = (row[6] != nullptr) ? std::atoi(row[6]) : 0;
                c.like_count = (row[7] != nullptr) ? static_cast<unsigned int>(std::atoi(row[7])) : 0;
                c.favorite_count = (row[8] != nullptr) ? static_cast<unsigned int>(std::atoi(row[8])) : 0;
                c.created_at = (row[9] != nullptr) ? row[9] : "";
                c.updated_at = (row[10] != nullptr) ? row[10] : "";
                c.reply_to_user_name = (row[12] != nullptr) ? row[12] : "";
                // row[11] is author_name, not stored in Comment struct
                comments->push_back(c);
            }
            // Optional: retrieve reply counts for top-level comments (nested <= 2 levels)
            // This is a best-effort lookup; counts are not stored in Comment struct.
            std::vector<unsigned long long> top_level_ids;
            top_level_ids.reserve(comments->size());
            for (const auto& cc : *comments) {
                if (cc.parent_id == 0) top_level_ids.push_back(cc.id);
            }
            if (!top_level_ids.empty()) {
                std::ostringstream idlist;
                for (size_t i = 0; i < top_level_ids.size(); ++i) {
                    if (i) idlist << ",";
                    idlist << top_level_ids[i];
                }
                std::string group_sql = "select parent_id, count(*) as cnt from solution_comments where parent_id in (" + idlist.str() + ") group by parent_id";
                if (mysql_query(my.get(), group_sql.c_str()) == 0)
                {
                    MYSQL_RES* res2 = mysql_store_result(my.get());
                    if (res2 != nullptr)
                    {
                        // read and discard counts; UI can fetch via separate API if needed
                        mysql_free_result(res2);
                    }
                }
            }
            mysql_free_result(res);

            // Write to cache
            Json::Value root(Json::objectValue);
            root["total"] = *total_count;
            Json::Value array_value(Json::arrayValue);
            for (const auto& c : *comments)
            {
                Json::Value item;
                item["id"] = static_cast<Json::UInt64>(c.id);
                item["solution_id"] = static_cast<Json::UInt64>(c.solution_id);
                item["user_id"] = c.user_id;
                item["content"] = c.content;
                item["is_edited"] = c.is_edited;
                item["parent_id"] = static_cast<Json::UInt64>(c.parent_id);
                item["reply_to_user_id"] = c.reply_to_user_id;
                item["like_count"] = c.like_count;
                item["favorite_count"] = c.favorite_count;
                item["created_at"] = c.created_at;
                item["updated_at"] = c.updated_at;
                item["reply_to_user_name"] = c.reply_to_user_name;
                array_value.append(item);
            }
            root["comments"] = array_value;
            Json::FastWriter writer;
            std::string json_str = writer.write(root);
            _cache.SetStringByAnyKey(cache_key, json_str, _cache.BuildJitteredTtl(300, 60));
            logger(ns_log::INFO) << "Cache miss for comment list, written to cache " << cache_key;

            return true;
        }

        // Comments: create a new comment
        bool CreateComment(const Comment& comment, unsigned long long* comment_id = nullptr)
        {
            if (comment.solution_id == 0 || comment.user_id <= 0)
            {
                return false;
            }

            auto my = CreateConnection();
            if (!my)
            {
                return false;
            }

            std::string safe_content = EscapeSqlString(comment.content, my.get());
            // if replying to a comment, ensure parent exists and is part of the same solution
            if (comment.parent_id > 0)
            {
                unsigned long long parent_solution_id = 0;
                std::string get_sql = "select solution_id from solution_comments where id=" + std::to_string(comment.parent_id);
                if (mysql_query(my.get(), get_sql.c_str()) != 0)
                {
                    logger(ns_log::FATAL) << "MySql查询父评论所属题解错误!";
                    return false;
                }
                MYSQL_RES* res_parent = mysql_store_result(my.get());
                if (res_parent == nullptr)
                {
                    logger(ns_log::FATAL) << "MySql父评论所属题解结果集为空!";
                    return false;
                }
                MYSQL_ROW row_parent = mysql_fetch_row(res_parent);
                if (row_parent == nullptr || row_parent[0] == nullptr)
                {
                    mysql_free_result(res_parent);
                    return false;
                }
                try
                {
                    parent_solution_id = std::stoull(row_parent[0]);
                }
                catch (const std::exception&){
                    mysql_free_result(res_parent);
                    return false;
                }
                mysql_free_result(res_parent);
                if (parent_solution_id != comment.solution_id)
                {
                    // parent belongs to different solution
                    return false;
                }
            }
            std::ostringstream sql;
            // Insert with optional parent and reply_to fields
            sql << "insert into solution_comments (solution_id, user_id, content, is_edited, parent_id, reply_to_user_id, like_count, favorite_count, created_at, updated_at) values (" 
                << comment.solution_id << ", " << comment.user_id << ", '" << safe_content << "', 0, "
                << comment.parent_id << ", " << comment.reply_to_user_id << ", " << comment.like_count << ", " << comment.favorite_count << ", NOW(), NOW())";

            if (mysql_query(my.get(), sql.str().c_str()) != 0)
            {
                logger(ns_log::FATAL) << "MySql执行错误! errno=" << mysql_errno(my.get())
                                      << " error=" << mysql_error(my.get());
                return false;
            }

            if (comment_id != nullptr)
            {
                *comment_id = static_cast<unsigned long long>(mysql_insert_id(my.get()));
            }

            // Get the question_id for cache invalidation
            std::string question_id;
            {
                std::string q_sql = "select question_id from " + SolutionsTable() + " where id=" + std::to_string(comment.solution_id);
                if (mysql_query(my.get(), q_sql.c_str()) == 0)
                {
                    MYSQL_RES* q_res = mysql_store_result(my.get());
                    if (q_res != nullptr)
                    {
                        MYSQL_ROW q_row = mysql_fetch_row(q_res);
                        if (q_row != nullptr && q_row[0] != nullptr)
                        {
                            question_id = q_row[0];
                        }
                        mysql_free_result(q_res);
                    }
                }
            }

            // update comment count on the related solution
            if (comment.parent_id == 0)
            {
                std::ostringstream upd;
                upd << "update " << SolutionsTable() << " set comment_count = comment_count + 1 where id = "
                    << comment.solution_id;
                if (!ExecuteSql(upd.str()))
                {
                    return false;
                }

                // Invalidate solution list cache since comment_count changed
                if (!question_id.empty())
                {
                    auto list_key = _cache.BuildSolutionCacheKey(
                        question_id, 1, 10, "1",
                        Cache::CacheKey::PageType::kList,
                        SolutionStatus::approved, SolutionSort::latest);
                    _cache.DeleteStringByAnyKey(list_key->GetCacheKeyString(&_cache));
                    logger(ns_log::INFO) << "Invalidated solution list cache after comment creation for question " << question_id;
                }
            }

            // Invalidate reply list cache for the parent comment (all common page sizes)
            if (comment.parent_id > 0)
            {
                std::string reply_base = "reply:list:pid:" + std::to_string(comment.parent_id);
                _cache.DeleteStringByAnyKey(reply_base + ":page:1:size:50");
                _cache.DeleteStringByAnyKey(reply_base + ":page:1:size:20");
                _cache.DeleteStringByAnyKey(reply_base + ":page:1:size:1000000");
                logger(ns_log::INFO) << "Invalidated reply list cache for parent " << comment.parent_id;
            }

            // Invalidate comment list cache for this solution
            std::string comment_list_key_prefix = "comment:list:sid:" + std::to_string(comment.solution_id);
            _cache.DeleteStringByAnyKey(comment_list_key_prefix + ":page:1:size:10");
            _cache.DeleteStringByAnyKey(comment_list_key_prefix + ":page:1:size:20");
            _cache.DeleteStringByAnyKey(comment_list_key_prefix + ":page:1:size:50");
            logger(ns_log::INFO) << "Invalidated comment list cache for solution " << comment.solution_id;

            // Pre-warm: try to read and update the comment list cache with the new comment
            if (comment.parent_id == 0)
            {
                std::string warm_key = "comment:list:sid:" + std::to_string(comment.solution_id) + ":page:1:size:20";
                std::string warm_json;
                if (_cache.GetStringByAnyKey(warm_key, &warm_json))
                {
                    Json::CharReaderBuilder builder;
                    Json::Value warm_value;
                    std::istringstream ss(warm_json);
                    if (Json::parseFromStream(builder, ss, &warm_value, nullptr))
                    {
                        Json::Value new_comment;
                        new_comment["id"] = Json::UInt64(comment.id);
                        new_comment["solution_id"] = Json::UInt64(comment.solution_id);
                        new_comment["user_id"] = comment.user_id;
                        new_comment["content"] = comment.content;
                        new_comment["is_edited"] = comment.is_edited;
                        new_comment["parent_id"] = Json::UInt64(comment.parent_id);
                        new_comment["reply_to_user_id"] = comment.reply_to_user_id;
                        new_comment["like_count"] = comment.like_count;
                        new_comment["favorite_count"] = comment.favorite_count;
                        new_comment["created_at"] = comment.created_at;
                        new_comment["updated_at"] = comment.updated_at;
                        new_comment["reply_to_user_name"] = "";
                        if (warm_value.isMember("comments") && warm_value["comments"].isArray())
                        {
                            Json::Value updated_arr(Json::arrayValue);
                            updated_arr.append(new_comment);
                            for (const auto& item : warm_value["comments"])
                            {
                                updated_arr.append(item);
                            }
                            warm_value["comments"] = updated_arr;
                            if (warm_value.isMember("total"))
                            {
                                warm_value["total"] = warm_value["total"].asInt() + 1;
                            }
                            Json::FastWriter writer;
                            _cache.SetStringByAnyKey(warm_key, writer.write(warm_value),
                                                     _cache.BuildJitteredTtl(300, 60));
                            logger(ns_log::INFO) << "Pre-warmed comment list cache for solution " << comment.solution_id;
                        }
                    }
                }
            }

            return true;
        }

        // Comments: get a single comment by id
        bool GetCommentById(unsigned long long comment_id, Comment* comment)
        {
            if (comment == nullptr)
            {
                return false;
            }

            auto my = CreateConnection();
            if (!my)
            {
                return false;
            }

            std::ostringstream sql;
            sql << "select c.id, c.solution_id, c.user_id, c.content, c.is_edited, c.parent_id, c.reply_to_user_id, c.like_count, c.favorite_count, c.created_at, c.updated_at, u.name AS author_name, ru.name AS reply_to_user_name from solution_comments c LEFT JOIN users u ON c.user_id = u.uid LEFT JOIN users ru ON c.reply_to_user_id = ru.uid where c.id="
                << comment_id;

            if (mysql_query(my.get(), sql.str().c_str()) != 0)
            {
                logger(ns_log::FATAL) << "MySql评论详情查询错误!";
                return false;
            }

            MYSQL_RES* res = mysql_store_result(my.get());
            if (res == nullptr)
            {
                logger(ns_log::FATAL) << "MySql评论详情结果集为空!";
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
            comment->id = 0;
            comment->solution_id = 0;
            try
            {
                if (row[0] != nullptr) comment->id = std::stoull(row[0]);
                if (row[1] != nullptr) comment->solution_id = std::stoull(row[1]);
            }
            catch (const std::exception&) { comment->id = 0; comment->solution_id = 0; }
            comment->user_id = (row[2] != nullptr) ? std::atoi(row[2]) : 0;
            comment->content = (row[3] != nullptr) ? row[3] : "";
            comment->is_edited = (row[4] != nullptr) ? (std::atoi(row[4]) != 0) : false;
            comment->parent_id = (row[5] != nullptr) ? static_cast<unsigned long long>(std::stoull(row[5])) : 0;
            comment->reply_to_user_id = (row[6] != nullptr) ? std::atoi(row[6]) : 0;
            comment->like_count = (row[7] != nullptr) ? static_cast<unsigned int>(std::atoi(row[7])) : 0;
            comment->favorite_count = (row[8] != nullptr) ? static_cast<unsigned int>(std::atoi(row[8])) : 0;
            comment->created_at = (row[9] != nullptr) ? row[9] : "";
            comment->updated_at = (row[10] != nullptr) ? row[10] : "";
            comment->reply_to_user_name = (row[12] != nullptr) ? row[12] : "";
            // row[11] is author_name; not stored in Comment struct
            mysql_free_result(res);
            return true;
        }

        // Comments: get replies for a given top-level comment
        bool GetCommentReplies(unsigned long long parent_id, int page, int size,
                               std::vector<Comment>* replies, int* total_count)
        {
            if (replies == nullptr || total_count == nullptr)
            {
                return false;
            }

            int safe_page = std::max(1, page);
            int safe_size = std::max(1, size);
            int offset = (safe_page - 1) * safe_size;

            // 尝试从缓存获取回复列表
            std::string reply_cache_key = "reply:list:pid:" + std::to_string(parent_id)
                + ":page:" + std::to_string(safe_page) + ":size:" + std::to_string(safe_size);
            std::string cached_reply_json;
            if (_cache.GetStringByAnyKey(reply_cache_key, &cached_reply_json))
            {
                Json::CharReaderBuilder builder;
                Json::Value json_value;
                std::istringstream ss(cached_reply_json);
                if (Json::parseFromStream(builder, ss, &json_value, nullptr))
                {
                    if (json_value.isMember("total"))
                        *total_count = json_value["total"].asInt();
                    if (json_value.isMember("comments") && json_value["comments"].isArray())
                    {
                        for (const auto& item : json_value["comments"])
                        {
                            Comment c;
                            c.id = item["id"].asUInt64();
                            c.solution_id = item["solution_id"].asUInt64();
                            c.user_id = item["user_id"].asInt();
                            c.content = item["content"].asString();
                            c.is_edited = item["is_edited"].asBool();
                            c.parent_id = item["parent_id"].asUInt64();
                            c.reply_to_user_id = item["reply_to_user_id"].asInt();
                            c.like_count = item["like_count"].asUInt();
                            c.favorite_count = item["favorite_count"].asUInt();
                            c.created_at = item["created_at"].asString();
                            c.updated_at = item["updated_at"].asString();
                            c.reply_to_user_name = item["reply_to_user_name"].asString();
                            replies->push_back(c);
                        }
                    }
                    logger(ns_log::INFO) << "Cache hit for reply list " << reply_cache_key;
                    return true;
                }
            }

            auto my = CreateConnection();
            if (!my)
            {
                return false;
            }

            // total count for this parent comment
            std::string count_sql = "select count(*) from solution_comments where parent_id=" + std::to_string(parent_id);
            if (!QueryCount(count_sql, total_count))
            {
                return false;
            }
            if (*total_count <= 0)
            {
                replies->clear();
                return true;
            }

            std::ostringstream sql;
            sql << "select c.id, c.solution_id, c.user_id, c.content, c.is_edited, c.parent_id, c.reply_to_user_id, c.like_count, c.favorite_count, c.created_at, c.updated_at, u.name AS author_name, ru.name AS reply_to_user_name "
                << "from solution_comments c LEFT JOIN users u ON c.user_id = u.uid LEFT JOIN users ru ON c.reply_to_user_id = ru.uid "
                << "where c.parent_id=" << parent_id
                << " order by c.created_at ASC LIMIT " << safe_size << " OFFSET " << offset;

            if (mysql_query(my.get(), sql.str().c_str()) != 0)
            {
                logger(ns_log::FATAL) << "MySql查询题解评论回复错误!";
                return false;
            }
            MYSQL_RES* res = mysql_store_result(my.get());
            if (res == nullptr)
            {
                logger(ns_log::FATAL) << "MySql题解评论回复结果集为空!";
                return false;
            }
            int rows = mysql_num_rows(res);
            replies->clear();
            replies->reserve(rows);
            for (int i = 0; i < rows; ++i)
            {
                MYSQL_ROW row = mysql_fetch_row(res);
                if (row == nullptr) continue;
                Comment c;
                try
                {
                    c.id = (row[0] != nullptr) ? std::stoull(row[0]) : 0;
                    c.solution_id = (row[1] != nullptr) ? std::stoull(row[1]) : 0;
                }
                catch (const std::exception&) { c.id = 0; c.solution_id = 0; }
                c.user_id = (row[2] != nullptr) ? std::atoi(row[2]) : 0;
                c.content = (row[3] != nullptr) ? row[3] : "";
                c.is_edited = (row[4] != nullptr) ? (std::atoi(row[4]) != 0) : false;
                c.parent_id = (row[5] != nullptr) ? static_cast<unsigned long long>(std::stoull(row[5])) : 0;
                c.reply_to_user_id = (row[6] != nullptr) ? std::atoi(row[6]) : 0;
                c.like_count = (row[7] != nullptr) ? static_cast<unsigned int>(std::atoi(row[7])) : 0;
                c.favorite_count = (row[8] != nullptr) ? static_cast<unsigned int>(std::atoi(row[8])) : 0;
                c.created_at = (row[9] != nullptr) ? row[9] : "";
                c.updated_at = (row[10] != nullptr) ? row[10] : "";
                c.reply_to_user_name = (row[12] != nullptr) ? row[12] : "";
                replies->push_back(c);
            }
            mysql_free_result(res);

            // 序列化回复列表并写入缓存
            Json::Value cache_value;
            cache_value["total"] = *total_count;
            Json::Value reply_arr(Json::arrayValue);
            for (const auto& c : *replies)
            {
                Json::Value item;
                item["id"] = Json::UInt64(c.id);
                item["solution_id"] = Json::UInt64(c.solution_id);
                item["user_id"] = c.user_id;
                item["content"] = c.content;
                item["is_edited"] = c.is_edited;
                item["parent_id"] = Json::UInt64(c.parent_id);
                item["reply_to_user_id"] = c.reply_to_user_id;
                item["like_count"] = c.like_count;
                item["favorite_count"] = c.favorite_count;
                item["created_at"] = c.created_at;
                item["updated_at"] = c.updated_at;
                item["reply_to_user_name"] = c.reply_to_user_name;
                reply_arr.append(item);
            }
            cache_value["comments"] = reply_arr;
            Json::FastWriter writer;
            _cache.SetStringByAnyKey(reply_cache_key, writer.write(cache_value),
                                     _cache.BuildJitteredTtl(120, 30));
            logger(ns_log::INFO) << "Cache miss for reply list, written to cache " << reply_cache_key;

            return true;
        }

        // Comments: update a comment (ownership check included)
        bool UpdateComment(unsigned long long comment_id, int user_id, const std::string& content)
        {
            auto my = CreateConnection();
            if (!my)
            {
                return false;
            }

            std::string safe_content = EscapeSqlString(content, my.get());
            std::ostringstream sql;
            sql << "update solution_comments set content='" << safe_content
                << "', is_edited = 1, updated_at = NOW() where id = " << comment_id
                << " and user_id = " << user_id;

            if (mysql_query(my.get(), sql.str().c_str()) != 0)
            {
                logger(ns_log::FATAL) << "MySql更新评论错误!";
                return false;
            }
            if (mysql_affected_rows(my.get()) == 0)
            {
                // no rows updated => wrong owner or not found
                return false;
            }
            return true;
        }

        // Comments: delete a comment (admin override supported) with cascade delete for replies
        bool DeleteComment(unsigned long long comment_id, int user_id, bool is_admin = false)
        {
            // first fetch the related solution_id
            auto my = CreateConnection();
            if (!my)
            {
                return false;
            }

            unsigned long long solution_id = 0;
            unsigned long long deleted_parent_id = 0;
            {
                std::string get_sql = "select solution_id,parent_id from solution_comments where id=" + std::to_string(comment_id);
                if (mysql_query(my.get(), get_sql.c_str()) != 0)
                {
                    logger(ns_log::FATAL) << "MySql查询评论所属题解错误! errno=" << mysql_errno(my.get())
                                          << " error=" << mysql_error(my.get());
                    return false;
                }
                MYSQL_RES* res = mysql_store_result(my.get());
                if (res == nullptr)
                {
                    logger(ns_log::FATAL) << "MySql评论所属题解查询结果集为空!";
                    return false;
                }
                MYSQL_ROW row = mysql_fetch_row(res);
                if (row != nullptr) {
                    if (row[0] != nullptr) {
                        try { solution_id = std::stoull(row[0]); }
                        catch (const std::exception& e) {
                            logger(ns_log::ERROR) << "stoull failed for solution_id: " << e.what();
                            mysql_free_result(res);
                            return false;
                        }
                    }
                    if (row[1] != nullptr) {
                        try { deleted_parent_id = std::stoull(row[1]); }
                        catch (...) { deleted_parent_id = 0; }
                    }
                }
                mysql_free_result(res);
            }
            if (solution_id == 0)
            {
                return false;
            }

            // count children for cascade delete
            int child_count = 0;
            {
                std::string cnt_sql = "select count(*) from solution_comments where parent_id=" + std::to_string(comment_id);
                if (!QueryCount(cnt_sql, &child_count))
                {
                    return false;
                }
            }

            // delete children first
            std::ostringstream del_children;
            del_children << "delete from solution_comments where parent_id=" << comment_id;
            if (mysql_query(my.get(), del_children.str().c_str()) != 0)
            {
                logger(ns_log::FATAL) << "MySql删除子评论错误! errno=" << mysql_errno(my.get())
                                      << " error=" << mysql_error(my.get());
                return false;
            }
            // delete main comment
            std::ostringstream del_sql;
            if (is_admin)
            {
                del_sql << "delete from solution_comments where id=" << comment_id;
            }
            else
            {
                del_sql << "delete from solution_comments where id=" << comment_id << " and user_id=" << user_id;
            }
            if (mysql_query(my.get(), del_sql.str().c_str()) != 0)
            {
                logger(ns_log::FATAL) << "MySql删除评论错误! errno=" << mysql_errno(my.get())
                                      << " error=" << mysql_error(my.get());
                return false;
            }
            // Properly consume the DELETE result to keep the connection clean
            {
                MYSQL_RES* discard = mysql_store_result(my.get());
                if (discard != nullptr)
                {
                    mysql_free_result(discard);
                }
            }

            // Get the question_id for cache invalidation
            std::string question_id;
            {
                std::string q_sql = "select question_id from " + SolutionsTable() + " where id=" + std::to_string(solution_id);
                if (mysql_query(my.get(), q_sql.c_str()) == 0)
                {
                    MYSQL_RES* q_res = mysql_store_result(my.get());
                    if (q_res != nullptr)
                    {
                        MYSQL_ROW q_row = mysql_fetch_row(q_res);
                        if (q_row != nullptr && q_row[0] != nullptr)
                        {
                            question_id = q_row[0];
                        }
                        mysql_free_result(q_res);
                    }
                }
            }

            // Only decrement for top-level comments (replies are not counted)
            if (deleted_parent_id == 0) {
                std::ostringstream upd_sql;
                upd_sql << "update " << SolutionsTable() << " set comment_count = GREATEST(comment_count - 1, 0) where id = "
                        << solution_id;
                if (!ExecuteSql(upd_sql.str()))
                {
                    return false;
                }
            }

            // Invalidate solution list cache since comment_count changed
            if (!question_id.empty())
            {
                auto list_key = _cache.BuildSolutionCacheKey(
                    question_id, 1, 10, "1",
                    Cache::CacheKey::PageType::kList,
                    SolutionStatus::approved, SolutionSort::latest);
                _cache.DeleteStringByAnyKey(list_key->GetCacheKeyString(&_cache));
                logger(ns_log::INFO) << "Invalidated solution list cache after comment deletion for question " << question_id;
            }

            // Invalidate reply cache for the deleted comment's own replies
            _cache.DeleteStringByAnyKey("reply:list:pid:" + std::to_string(comment_id) + ":page:1:size:50");

            return true;
        }

        // Admin: check if a uid belongs to an admin account
        bool GetAdminByUid(int uid)
        {
            AdminAccount admin;
            std::string sql = "select admin_id,password_hash,uid,role,created_at from " + AdminAccountsTable() + " where uid=" + std::to_string(uid);
            return QueryAdminAccount(sql, &admin);
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

            // Invalidate solution detail cache since like/favorite count changed
            auto detail_key = _cache.BuildSolutionDetailCacheKey(solution_id);
            _cache.DeleteStringByAnyKey(detail_key->GetCacheKeyString(&_cache));
            logger(ns_log::INFO) << "Invalidated solution detail cache after action toggle for solution " << solution_id;

                return true;
        }

        // Comments: batch get user actions on comments
        bool GetCommentActions(const std::vector<unsigned long long>& comment_ids,
                               int user_id,
                               std::map<unsigned long long, std::map<std::string, bool>>* actions)
        {
            if (actions == nullptr)
            {
                return false;
            }
            if (comment_ids.empty())
            {
                return true;
            }
            auto my = CreateConnection();
            if (!my)
            {
                return false;
            }
            std::ostringstream ids_stream;
            for (size_t i = 0; i < comment_ids.size(); ++i)
            {
                if (i > 0) ids_stream << ",";
                ids_stream << comment_ids[i];
            }
            std::ostringstream sql;
            sql << "select comment_id, action_type from comment_actions where user_id=" << user_id
                << " and comment_id in (" << ids_stream.str() << ")";
            if (mysql_query(my.get(), sql.str().c_str()) != 0)
            {
                logger(ns_log::FATAL) << "MySql查询评论用户交互错误!";
                return false;
            }
            MYSQL_RES* res = mysql_store_result(my.get());
            if (res == nullptr)
            {
                logger(ns_log::FATAL) << "MySql评论用户交互结果集为空!";
                return false;
            }
            for (int i = 0; i < mysql_num_rows(res); ++i)
            {
                MYSQL_ROW row = mysql_fetch_row(res);
                if (row == nullptr) continue;
                unsigned long long cid = 0;
                std::string atype = row[1] ? row[1] : "";
                try {
                    cid = std::stoull(row[0]);
                } catch (...) {
                    cid = 0;
                }
                if (cid != 0) {
                    (*actions)[cid][atype] = true;
                }
            }
            mysql_free_result(res);
            return true;
        }

        // Comments: toggle action on a comment (like/favorite)
        bool ToggleCommentAction(unsigned long long comment_id, int user_id, const std::string& action_type,
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

            // 尝试从缓存获取用户交互状态
            std::string action_cache_key = "action:user:" + std::to_string(user_id) + ":comment:" + std::to_string(comment_id);
            std::string cached_action_json;
            bool cached_exists = false;
            bool cached_found = false;
            if (_cache.GetStringByAnyKey(action_cache_key, &cached_action_json))
            {
                Json::CharReaderBuilder reader;
                Json::Value cv;
                std::istringstream css(cached_action_json);
                if (Json::parseFromStream(reader, css, &cv, nullptr))
                {
                    if (action_type == "like" && cv.isMember("like"))
                    {
                        cached_exists = cv["like"].asBool();
                        cached_found = true;
                    }
                    else if (action_type == "favorite" && cv.isMember("favorite"))
                    {
                        cached_exists = cv["favorite"].asBool();
                        cached_found = true;
                    }
                }
            }

            int exist_rows = 0;
            if (cached_found)
            {
                exist_rows = cached_exists ? 1 : 0;
            }
            else
            {
                std::ostringstream check_sql;
                check_sql << "select id from comment_actions where comment_id=" << comment_id
                          << " and user_id=" << user_id
                          << " and action_type='" << safe_action << "'";

                if (mysql_query(my.get(), check_sql.str().c_str()) != 0)
                {
                    logger(ns_log::FATAL) << "MySql查询评论交互记录错误!";
                    return false;
                }
                MYSQL_RES* res = mysql_store_result(my.get());
                if (res == nullptr)
                {
                    logger(ns_log::FATAL) << "MySql评论交互记录结果集为空!";
                    return false;
                }
                exist_rows = mysql_num_rows(res);
                mysql_free_result(res);
            }

            if (exist_rows > 0)
            {
                std::ostringstream del_sql;
                del_sql << "delete from comment_actions where comment_id=" << comment_id
                        << " and user_id=" << user_id
                        << " and action_type='" << safe_action << "'";
                if (mysql_query(my.get(), del_sql.str().c_str()) != 0)
                {
                    logger(ns_log::FATAL) << "MySql删除评论交互记录错误!";
                    return false;
                }

                std::ostringstream dec_sql;
                dec_sql << "update solution_comments set " << count_column << " = " << count_column << " - 1"
                        << " where id = " << comment_id;
                if (mysql_query(my.get(), dec_sql.str().c_str()) != 0)
                {
                    logger(ns_log::FATAL) << "MySql更新评论计数错误!";
                    return false;
                }

                *now_active = false;
            }
            else
            {
                std::ostringstream ins_sql;
                ins_sql << "insert into comment_actions (comment_id, user_id, action_type, created_at) values ("
                        << comment_id << ", " << user_id << ", '" << safe_action << "', NOW())";
                if (mysql_query(my.get(), ins_sql.str().c_str()) != 0)
                {
                    logger(ns_log::FATAL) << "MySql插入评论交互记录错误!";
                    return false;
                }

                std::ostringstream inc_sql;
                inc_sql << "update solution_comments set " << count_column << " = " << count_column << " + 1"
                        << " where id = " << comment_id;
                if (mysql_query(my.get(), inc_sql.str().c_str()) != 0)
                {
                    logger(ns_log::FATAL) << "MySql更新评论计数错误!";
                    return false;
                }

                *now_active = true;
            }

            // 更新交互状态缓存
            Json::Value action_state;
            if (!cached_action_json.empty())
            {
                Json::CharReaderBuilder reader;
                std::istringstream css(cached_action_json);
                Json::parseFromStream(reader, css, &action_state, nullptr);
            }
            action_state[action_type] = !*now_active ? false : true;
            Json::FastWriter action_writer;
            _cache.SetStringByAnyKey(action_cache_key, action_writer.write(action_state),
                                     _cache.BuildJitteredTtl(300, 60));

            std::ostringstream cnt_sql;
            cnt_sql << "select " << count_column << " from solution_comments where id = " << comment_id;
            if (mysql_query(my.get(), cnt_sql.str().c_str()) != 0)
            {
                logger(ns_log::FATAL) << "MySql查询评论计数错误!";
                return false;
            }
            MYSQL_RES* cnt_res = mysql_store_result(my.get());
            if (cnt_res == nullptr)
            {
                logger(ns_log::FATAL) << "MySql评论计数结果集为空!";
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
                long long sid = 0;
                try { sid = std::stoll(row[0]); } catch (const std::exception&) { sid = 0; }
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
                logger(ns_log::FATAL)<<"MySql查询错误: "<<mysql_error(my.get());
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
            std::string sql = "select uid,name,email,avatar_path,create_time,last_login,password_algo from " + oj_users + " where email='" + safe_email + "'";
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
            std::string sql = "select uid,name,email,avatar_path,create_time,last_login,password_algo from " + oj_users + " where uid=" + std::to_string(uid);
            bool ok = QueryUser(sql, user);
            if (ok) {
                _cache.SetStringByAnyKey("avatar:user:" + std::to_string(uid), user->avatar_path, _cache.BuildJitteredTtl(3600, 600));
            }
            return ok;
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
            page_sql << "select uid,name,email,avatar_path,create_time,last_login from " << oj_users
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

        bool UpdateUserAvatar(int uid, const std::string& avatar_path)
        {
            auto my = CreateConnection();
            if (!my)
                return false;

            std::string safe_path = EscapeSqlString(avatar_path, my.get());
            std::ostringstream sql;
            sql << "update " << oj_users << " set avatar_path='" << safe_path
                << "' where uid=" << uid;
            return ExecuteSql(sql.str());
        }

        bool GetUserByName(const std::string& name, User* user)
        {
            auto my = CreateConnection();
            if(!my)
                return false;

            std::string safe_name = EscapeSqlString(name, my.get());
            std::string sql = "select uid,name,email,avatar_path,create_time,last_login,password_algo from " + oj_users + " where name='" + safe_name + "'";
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
            if (!ExecuteSql(sql.str()))
                return false;

            _cache.DeleteStringByAnyKey("submit:user:" + std::to_string(user_id) + ":q:" + question_id);
            _cache.DeleteStringByAnyKey("stats:user:" + std::to_string(user_id));
            return true;
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

        // Feature 2.4: Get user submissions for a specific question
        bool GetUserSubmitsByQuestion(int user_id, const std::string& question_id, Json::Value* submits)
        {
            if (submits == nullptr)
                return false;

            // 先查缓存
            std::string cache_key = "submit:user:" + std::to_string(user_id) + ":q:" + question_id;
            std::string cached;
            if (_cache.GetStringByAnyKey(cache_key, &cached))
            {
                Json::CharReaderBuilder builder;
                Json::Value cached_val;
                std::istringstream ss(cached);
                if (Json::parseFromStream(builder, ss, &cached_val, nullptr) && cached_val.isArray())
                {
                    *submits = cached_val;
                    logger(ns_log::INFO) << "Cache hit for user submits user=" << user_id << " q=" << question_id;
                    return true;
                }
            }

            auto my = CreateConnection();
            if (!my)
                return false;

            std::string safe_qid = EscapeSqlString(question_id, my.get());
            std::ostringstream sql;
            sql << "select result_json, is_pass, action_time from user_submits"
                << " where user_id=" << user_id
                << " and question_id='" << safe_qid << "'"
                << " order by action_time desc";

            if (mysql_query(my.get(), sql.str().c_str()) != 0)
            {
                logger(ns_log::FATAL) << "MySql查询提交记录错误!";
                return false;
            }

            MYSQL_RES* res = mysql_store_result(my.get());
            if (res == nullptr)
            {
                logger(ns_log::FATAL) << "MySql提交记录结果集为空!";
                return false;
            }

            submits->clear();
            for (int i = 0; i < mysql_num_rows(res); ++i)
            {
                MYSQL_ROW row = mysql_fetch_row(res);
                if (row == nullptr) continue;
                Json::Value item;
                item["result_json"] = row[0] ? row[0] : "";
                item["is_pass"] = (row[1] != nullptr && row[1][0] != 0);
                item["action_time"] = row[2] ? row[2] : "";
                submits->append(item);
            }
            mysql_free_result(res);

            // 回写缓存
            Json::FastWriter writer;
            std::string json_str = writer.write(*submits);
            _cache.SetStringByAnyKey(cache_key, json_str, _cache.BuildJitteredTtl(600, 120));

            return true;
        }

        // Feature 2.5a: Get sample tests for a question
        bool GetSampleTestsByQuestionId(const std::string& question_id, Json::Value* tests)
        {
            if (tests == nullptr)
                return false;

            auto my = CreateConnection();
            if (!my)
                return false;

            std::string safe_qid = EscapeSqlString(question_id, my.get());
            std::string sql = "select id, question_id, `in`, `out`, is_sample from " + oj_tests +
                              " where question_id='" + safe_qid + "' and is_sample=1 order by id asc";

            if (mysql_query(my.get(), sql.c_str()) != 0)
            {
                logger(ns_log::FATAL) << "MySql查询测试用例错误!";
                return false;
            }

            MYSQL_RES* res = mysql_store_result(my.get());
            if (res == nullptr)
            {
                logger(ns_log::FATAL) << "MySql测试用例结果集为空!";
                return false;
            }

            tests->clear();
            for (int i = 0; i < mysql_num_rows(res); ++i)
            {
                MYSQL_ROW row = mysql_fetch_row(res);
                if (row == nullptr) continue;
                Json::Value item;
                item["id"] = row[0] ? std::atoi(row[0]) : 0;
                item["question_id"] = row[1] ? row[1] : "";
                item["input"] = row[2] ? row[2] : "";
                item["output"] = row[3] ? row[3] : "";
                item["is_sample"] = (row[4] && std::atoi(row[4]) != 0) ? true : false;
                tests->append(item);
            }
            mysql_free_result(res);
            return true;
        }

        // Feature 2.5: Get a single test case by ID (for custom test execution)
        bool GetTestById(int test_id, const std::string& question_id, std::string* test_input, std::string* test_output)
        {
            if (test_input == nullptr || test_output == nullptr)
                return false;

            auto my = CreateConnection();
            if (!my)
                return false;

            std::string safe_qid = EscapeSqlString(question_id, my.get());
            std::ostringstream sql;
            sql << "select `in`, `out` from " << oj_tests
                << " where id=" << test_id
                << " and question_id='" << safe_qid << "'";

            if (mysql_query(my.get(), sql.str().c_str()) != 0)
            {
                logger(ns_log::FATAL) << "MySql查询测试用例错误!";
                return false;
            }

            MYSQL_RES* res = mysql_store_result(my.get());
            if (res == nullptr)
            {
                logger(ns_log::FATAL) << "MySql测试用例结果集为空!";
                return false;
            }

            MYSQL_ROW row = mysql_fetch_row(res);
            if (row == nullptr || row[0] == nullptr || row[1] == nullptr)
            {
                mysql_free_result(res);
                return false;
            }

            *test_input = row[0];
            *test_output = row[1];
            mysql_free_result(res);
            return true;
        }

        // Feature 2.6: Get user statistics
        bool GetUserStats(int user_id, Json::Value* stats)
        {
            if (stats == nullptr)
                return false;

            std::string cache_key = "stats:user:" + std::to_string(user_id);
            std::string cached;
            if (_cache.GetStringByAnyKey(cache_key, &cached))
            {
                Json::CharReaderBuilder builder;
                std::istringstream ss(cached);
                if (Json::parseFromStream(builder, ss, stats, nullptr))
                {
                    logger(ns_log::INFO) << "Cache hit for user stats user=" << user_id;
                    return true;
                }
            }

            auto my = CreateConnection();
            if (!my)
                return false;

            std::ostringstream count_sql;
            count_sql << "select count(*) from user_submits where user_id=" << user_id;
            int total_submits = 0;
            if (!QueryCount(count_sql.str(), &total_submits))
                return false;

            std::ostringstream pass_sql;
            pass_sql << "select count(distinct question_id) from user_submits where user_id=" << user_id << " and is_pass=1";
            int passed_questions = 0;
            if (!QueryCount(pass_sql.str(), &passed_questions))
                return false;

            std::ostringstream passed_submits_sql;
            passed_submits_sql << "select count(*) from user_submits where user_id=" << user_id << " and is_pass=1";
            int passed_submits = 0;
            if (!QueryCount(passed_submits_sql.str(), &passed_submits))
                return false;

            double accuracy = total_submits > 0 ? static_cast<double>(passed_submits) / static_cast<double>(total_submits) : 0.0;

            (*stats)["total_submits"] = total_submits;
            (*stats)["passed_questions"] = passed_questions;
            (*stats)["passed_submits"] = passed_submits;

            Json::Value accuracy_val(accuracy);
            (*stats)["accuracy"] = accuracy_val;

            std::ostringstream recent_sql;
            recent_sql << "select us.question_id, us.is_pass, us.action_time, us.result_json, q.title, q.star"
                       << " from user_submits us"
                       << " left join " << oj_questions << " q on us.question_id = q.id"
                       << " where us.user_id=" << user_id
                       << " order by us.action_time desc limit 20";

            if (mysql_query(my.get(), recent_sql.str().c_str()) != 0)
            {
                logger(ns_log::FATAL) << "MySql查询最近提交错误!";
                return false;
            }

            MYSQL_RES* res = mysql_store_result(my.get());
            if (res == nullptr)
            {
                logger(ns_log::FATAL) << "MySql最近提交结果集为空!";
                return false;
            }

            Json::Value recent_arr(Json::arrayValue);
            for (int i = 0; i < mysql_num_rows(res); ++i)
            {
                MYSQL_ROW row = mysql_fetch_row(res);
                if (row == nullptr) continue;
                Json::Value item;
                item["question_id"] = row[0] ? row[0] : "";
                item["is_pass"] = (row[1] != nullptr && row[1][0] != 0);
                item["action_time"] = row[2] ? row[2] : "";
                item["result_json"] = row[3] ? row[3] : "";
                item["title"] = row[4] ? row[4] : "";
                item["star"] = row[5] ? row[5] : "";
                recent_arr.append(item);
            }
            mysql_free_result(res);
            (*stats)["recent_submits"] = recent_arr;

            Json::FastWriter writer;
            // Only cache if there's actual data to prevent stale empty results from persisting
            if (total_submits > 0)
            {
                _cache.SetStringByAnyKey(cache_key, writer.write(*stats), _cache.BuildJitteredTtl(180, 60));
            }

            return true;
        }

    private:
        Cache _cache;
    };

};
