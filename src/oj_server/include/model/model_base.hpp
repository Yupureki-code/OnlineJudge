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

        Cache _cache;

    public:
        const std::string& SolutionsTable() const
        {
            static const std::string table = "solutions";
            return table;
        }
    };

}
