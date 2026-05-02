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
#include <mysql/mysql.h>

namespace ns_model
{
    using namespace ns_log;
    using namespace ns_cache;

    class ModelBase
    {
    protected:
        // 判断字符串是否全为数字
        bool IsAllDigits(const std::string& s) const
        {
            return !s.empty() && std::all_of(s.begin(), s.end(), [](unsigned char ch) { return std::isdigit(ch); });
        }
        // 去除首尾空白字符
        std::string TrimCopy(const std::string& s) const
        {
            size_t begin = 0, end = s.size();
            while (begin < end && std::isspace(static_cast<unsigned char>(s[begin]))) ++begin;
            while (end > begin && std::isspace(static_cast<unsigned char>(s[end - 1]))) --end;
            return s.substr(begin, end - begin);
        }
        // 转小写
        std::string LowerAscii(const std::string& s) const
        {
            std::string out = s;
            std::transform(out.begin(), out.end(), out.begin(), [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
            return out;
        }

    public:
        using MySqlConn = std::unique_ptr<MYSQL, void(*)(MYSQL*)>;

        // 线程局部数据库连接（复用连接，减少TCP握手开销）
        MYSQL* GetThreadConnection()
        {
            thread_local MYSQL* conn = nullptr;
            thread_local std::chrono::steady_clock::time_point last_used;

            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - last_used).count();

            if (conn && elapsed > 300 && mysql_ping(conn) != 0)
            {
                mysql_close(conn);
                conn = nullptr;
            }

            if (!conn)
            {
                conn = mysql_init(nullptr);
                if (mysql_real_connect(conn, host.c_str(), user.c_str(), passwd.c_str(),
                                       db.c_str(), port, nullptr, 0) == nullptr)
                {
                    logger(ns_log::FATAL) << "MySql连接失败!";
                    mysql_close(conn);
                    conn = nullptr;
                }
            }
            last_used = now;
            return conn;
        }

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

        //执行SQL查询，返回结果集指针。调用方需自行 mysql_free_result
        MYSQL_RES* QueryMySql(MYSQL* my, const std::string& sql, const std::string& error_msg)
        {
            if (mysql_query(my, sql.c_str()) != 0)
            {
                logger(ns_log::FATAL) << error_msg << ": " << mysql_error(my);
                return nullptr;
            }
            MYSQL_RES* res = mysql_store_result(my);
            if (res == nullptr)
            {
                logger(ns_log::FATAL) << error_msg << " (结果集为空)";
                return nullptr;
            }
            return res;
        }

        // 缓存指标结构体:记录题目列表和题目详情这两条查询链路的运行指标。
        struct CacheMetrics
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

        bool QueryCount(const std::string& sql, int* count)
        {
            if (count == nullptr)
                return false;

            auto my = CreateConnection();
            if(!my)
                return false;

            MYSQL_RES* res = QueryMySql(my.get(), sql, "MySql统计查询错误");
            if (!res) return false;

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

        //查询用户基础信息
        bool QueryUser(const std::string& sql, User* user)
        {
            auto my = CreateConnection();
            if(!my)
                return false;
            MYSQL_RES* res = QueryMySql(my.get(), sql, "MySql查询错误");
            if (!res) return false;
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
            MYSQL_RES* res = QueryMySql(my.get(), sql, "MySql查询错误");
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

            *password_hash = row[0] == nullptr ? "" : row[0];
            *password_algo = row[1] == nullptr ? "" : row[1];
            mysql_free_result(res);
            return true;
        }

        Cache _cache;

    public:
        const std::string& SolutionsTable() const
        {
            static const std::string table = "solutions";
            return table;
        }
    };

}
