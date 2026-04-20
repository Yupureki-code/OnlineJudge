#pragma once

#include <cstdio>
#include <assert.h>
#include <cstdlib>
#include <vector>
#include "../../comm/logstrategy.hpp"
#include "../../comm/comm.hpp"
#include <mysql/mysql.h>

// model: 主要用来和数据进行交互，对外提供访问数据的接口

namespace ns_model
{
    using namespace ns_log;

    //题目属性
    struct Question
    {
        std::string number; //题号
        std::string title;  //标题
        std::string star;   //难度
        std::string desc;   //描述
        int cpu_limit;      //时间限制
        int memory_limit;   //内存限制
        std::string create_time; //创建时间
        std::string update_time; //更新时间
    };

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
        //查询Mysql
        bool QueryMySql(const std::string& sql,std::vector<Question>* qs)
        {
            MYSQL* my = mysql_init(nullptr);
            if(mysql_real_connect(my, host.c_str(), user.c_str(), passwd.c_str(), db.c_str(), port, nullptr, 0) == nullptr)
                return false;
            if(mysql_query(my, sql.c_str()) != 0)
            {
                ns_log::logger(ns_log::FATAL)<<"MySql查询错误!";
                return false;
            }
            MYSQL_RES* res = mysql_store_result(my);
            int rows = mysql_num_rows(res);
            int cols = mysql_num_fields(res);
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
                qs->push_back(q);
            }
            mysql_free_result(res);
            mysql_close(my);
            return true;
        }

        //查询用户
        bool QueryUser(const std::string& sql, User* user)
        {
            MYSQL* my = mysql_init(nullptr);
            if(mysql_real_connect(my, host.c_str(), ::user.c_str(), passwd.c_str(), db.c_str(), port, nullptr, 0) == nullptr)
                return false;
            if(mysql_query(my, sql.c_str()) != 0)
            {
                ns_log::logger(ns_log::FATAL)<<"MySql查询错误!";
                mysql_close(my);
                return false;
            }
            MYSQL_RES* res = mysql_store_result(my);
            int rows = mysql_num_rows(res);
            if(rows != 1)
            {
                mysql_free_result(res);
                mysql_close(my);
                return false;
            }
            MYSQL_ROW row = mysql_fetch_row(res);
            user->uid = atoi(row[0]);
            user->name = row[1];
            user->email = row[4];
            user->create_time = row[2];
            user->last_login = row[3];
            mysql_free_result(res);
            mysql_close(my);
            return true;
        }

        //执行SQL语句（用于插入、更新等操作）
        bool ExecuteSql(const std::string& sql)
        {
            MYSQL* my = mysql_init(nullptr);
            if(mysql_real_connect(my, host.c_str(), user.c_str(), passwd.c_str(), db.c_str(), port, nullptr, 0) == nullptr)
                return false;
            if(mysql_query(my, sql.c_str()) != 0)
            {
                ns_log::logger(ns_log::FATAL)<<"MySql执行错误!";
                mysql_close(my);
                return false;
            }
            mysql_close(my);
            return true;
        }

    public:
        //题库:得到一页内全部的题目
        bool GetAllQuestions(std::vector<Question>* qs)
        {
            std::string sql = "select * from ";
            sql += oj_questions;
            return QueryMySql(sql, qs);
        }
        //题目:获得单个题目
        bool GetOneQuestion(const std::string& number,Question* q)
        {
            std::string sql = "select * from ";
            sql += oj_questions;
            sql += " where id=" + number;
            std::vector<Question> v;
            QueryMySql(sql, &v);
            if(v.size() != 1)
                return false;
            *q = v[0];
            return true;
        }

        //用户:检查用户是否存在
        bool CheckUser(const std::string& email)
        {
            std::string sql = "select * from " + oj_users + " where email='" + email + "'";
            MYSQL* my = mysql_init(nullptr);
            if(mysql_real_connect(my, host.c_str(), user.c_str(), passwd.c_str(), db.c_str(), port, nullptr, 0) == nullptr)
                return false;
            if(mysql_query(my, sql.c_str()) != 0)
            {
                ns_log::logger(ns_log::FATAL)<<"MySql查询错误!";
                mysql_close(my);
                return false;
            }
            MYSQL_RES* res = mysql_store_result(my);
            int rows = mysql_num_rows(res);
            mysql_free_result(res);
            mysql_close(my);
            return rows > 0;
        }

        //用户:创建新用户
        bool CreateUser(const std::string& name, const std::string& email)
        {
            std::string sql = "insert into " + oj_users + " (name, email) values ('" + name + "', '" + email + "')";
            return ExecuteSql(sql);
        }

        //用户:获取用户信息
        bool GetUser(const std::string& email, User* user)
        {
            std::string sql = "select * from " + oj_users + " where email='" + email + "'";
            return QueryUser(sql, user);
        }

        //用户:更新最后登录时间
        bool UpdateLastLogin(const std::string& email)
        {
            std::string sql = "update " + oj_users + " set last_login=NOW() where email='" + email + "'";
            return ExecuteSql(sql);
        }

        //用户:通过ID获取用户信息
        bool GetUserById(int uid, User* user)
        {
            std::string sql = "select * from " + oj_users + " where uid=" + std::to_string(uid);
            return QueryUser(sql, user);
        }

    };
};