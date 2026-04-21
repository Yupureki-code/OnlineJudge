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
    struct User
    {
        int uid; //用户ID
        std::string name; //用户名
        std::string email; //邮箱
        std::string create_time; //创建时间
        std::string last_login; //最后登录时间
    };

    class Model
    {
    private:
        struct CacheMetrics
        {
            std::atomic<long long> list_requests{0};
            std::atomic<long long> list_hits{0};
            std::atomic<long long> list_misses{0};
            std::atomic<long long> list_db_fallbacks{0};
            std::atomic<long long> list_total_ms{0};

            std::atomic<long long> detail_requests{0};
            std::atomic<long long> detail_hits{0};
            std::atomic<long long> detail_misses{0};
            std::atomic<long long> detail_db_fallbacks{0};
            std::atomic<long long> detail_total_ms{0};
        };

        static CacheMetrics& Metrics()
        {
            static CacheMetrics m;
            return m;
        }

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

        QueryStruct NormalizeQuestionQuery(const QueryStruct& raw_query)
        {
            QueryStruct normalized;
            normalized.id = TrimCopy(raw_query.id);
            normalized.title = TrimCopy(raw_query.title);
            normalized.difficulty = LowerAscii(TrimCopy(raw_query.difficulty));

            if (!normalized.id.empty() && !IsAllDigits(normalized.id))
            {
                normalized.id.clear();
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

        std::string BuildQuestionWhereClause(const QueryStruct& query_hash, MYSQL* my)
        {
            std::vector<std::string> clauses;

            // both 模式映射后会产生 id/title 同值，且 id 为纯数字，此时保持 OR 语义。
            if (!query_hash.id.empty() &&
                !query_hash.title.empty() &&
                query_hash.id == query_hash.title &&
                IsAllDigits(query_hash.id))
            {
                std::string safe_title = EscapeSqlString(query_hash.title, my);
                clauses.push_back("(id=" + query_hash.id + " or title like '%" + safe_title + "%')");
            }
            else
            {
                if (!query_hash.id.empty())
                {
                    if (IsAllDigits(query_hash.id))
                    {
                        clauses.push_back("id=" + query_hash.id);
                    }
                    else
                    {
                        clauses.push_back("1=0");
                    }
                }

                if (!query_hash.title.empty())
                {
                    std::string safe_title = EscapeSqlString(query_hash.title, my);
                    clauses.push_back("title like '%" + safe_title + "%'");
                }
            }

            if (!query_hash.difficulty.empty())
            {
                std::string difficulty = query_hash.difficulty;
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

        //查询用户
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
            user->uid = atoi(row[0]);
            user->name = row[1];
            user->email = row[4];
            user->create_time = row[2];
            user->last_login = row[3];
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
                logger(ns_log::FATAL)<<"MySql执行错误!";
                return false;
            }
            return true;
        }

    public:
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
        };

        bool GetAllQuestions(std::vector<Question>* questions)
        {
            if (questions == nullptr)
                return false;

            std::string sql = "select * from ";
            sql += oj_questions;
            return QueryMySql(sql, *questions);
        }

        //题库:得到一页内全部的题目
        bool GetAllQuestions(int page,
                             int size,
                             const std::string& list_version,
                             std::vector<Question>& questions,
                             int* total_count,
                             int* total_pages,
                             const QueryStruct& query_hash = QueryStruct())
        {
            auto begin = std::chrono::steady_clock::now();
            if (total_count == nullptr || total_pages == nullptr)
            {
                auto end = std::chrono::steady_clock::now();
                long long cost_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
                RecordListMetrics(false, false, cost_ms);
                return false;
            }

            if (page < 1) page = 1;
            if (size < 1) size = 5;

            (void)list_version;

            QueryStruct normalized_query = NormalizeQuestionQuery(query_hash);
            std::string effective_list_version = _cache.GetListVersion();

            if(_cache.GetAllQuestions(page, size, effective_list_version, total_count, total_pages, questions, normalized_query))
            {
                logger(ns_log::INFO) << "Cache hit for question list page " << page;
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

            std::string where_clause = BuildQuestionWhereClause(normalized_query, my.get());

            std::string count_sql = "select count(*) from " + oj_questions + where_clause;
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
                questions.clear();
                _cache.SetAllQuestions(page, size, effective_list_version, *total_count, *total_pages, questions, normalized_query);
                auto end = std::chrono::steady_clock::now();
                long long cost_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
                RecordListMetrics(false, true, cost_ms);
                return true;
            }

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

            _cache.SetAllQuestions(safe_page, size, effective_list_version, *total_count, *total_pages, questions, normalized_query);
            auto end = std::chrono::steady_clock::now();
            long long cost_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
            RecordListMetrics(false, true, cost_ms);
            return true;
        }
        //题目:获得单个题目
        bool GetOneQuestion(const std::string& number,Question& q)
        {
            auto begin = std::chrono::steady_clock::now();
            if (!IsAllDigits(number))
            {
                auto end = std::chrono::steady_clock::now();
                long long cost_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
                RecordDetailMetrics(false, false, cost_ms);
                return false;
            }
            if(_cache.GetQuestion(number, q))
            {
                logger(ns_log::INFO) << "Cache hit for question " << number;
                auto end = std::chrono::steady_clock::now();
                long long cost_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
                RecordDetailMetrics(true, false, cost_ms);
                return true;
            }
            std::string sql = "select * from ";
            sql += oj_questions;
            sql += " where id=" + number;
            std::vector<Question> v;
            if (!QueryMySql(sql, v))
            {
                _cache.SetQuestionNotFound(number);
                auto end = std::chrono::steady_clock::now();
                long long cost_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
                RecordDetailMetrics(false, true, cost_ms);
                return false;
            }
            if(v.size() != 1)
            {
                _cache.SetQuestionNotFound(number);
                auto end = std::chrono::steady_clock::now();
                long long cost_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
                RecordDetailMetrics(false, true, cost_ms);
                return false;
            }
            q = v[0];
            _cache.SetQuestion(q);
            auto end = std::chrono::steady_clock::now();
            long long cost_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
            RecordDetailMetrics(false, true, cost_ms);
            return true;
        }

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
            _cache.SetQuestion(cached);
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

            _cache.SetQuestionNotFound(number);
            TouchQuestionListVersion();
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
            std::string sql = "insert into " + oj_users + " (name, email) values ('" + safe_name + "', '" + safe_email + "')";
            return ExecuteSql(sql);
        }

        //用户:获取用户信息
        bool GetUser(const std::string& email, User* user)
        {
            auto my = CreateConnection();
            if(!my)
                return false;

            std::string safe_email = EscapeSqlString(email, my.get());
            std::string sql = "select * from " + oj_users + " where email='" + safe_email + "'";
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
            std::string sql = "select * from " + oj_users + " where uid=" + std::to_string(uid);
            return QueryUser(sql, user);
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
            return s;
        }

        // 题目写路径完成后调用该接口，统一触发列表版本递增。
        std::string TouchQuestionListVersion()
        {
            std::string version = _cache.BumpListVersion();
            logger(ns_log::INFO) << "Question list version bumped to " << version;
            return version;
        }
    private:
        Cache _cache;
    };

};