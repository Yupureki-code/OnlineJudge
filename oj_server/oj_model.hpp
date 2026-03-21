#pragma once

#include <cstdio>
#include <assert.h>
#include <cstdlib>
#include <vector>
#include "../comm/logstrategy.hpp"
#include <mysql/mysql.h>

namespace ns_model
{
    using namespace ns_log;

    struct Question
    {
        std::string number;
        std::string title;
        std::string star;
        std::string desc;
        int cpu_limit;
        int memory_limit;
        std::string create_time;
        std::string update_time;
    };

    const std::string oj_questions = "questions";
    const std::string host = "127.0.0.1";
    const std::string user = "oj_server";
    const std::string passwd = "Myoj@2026071024";
    const std::string db = "myoj";
    const int port = 3306;

    class Model
    {
    private:
        bool QueryMySql(const std::string& sql,std::vector<Question>* qs)
        {
            MYSQL* my = mysql_init(nullptr);
            if(mysql_real_connect(my, host.c_str(), user.c_str(), passwd.c_str(), db.c_str(), port, nullptr, 0) == nullptr)
                return false;
            if(mysql_query(my, sql.c_str()) != 0)
            {
                logger(ns_log::FATAL)<<"MySql查询错误!";
                return false;
            }
            MYSQL_RES* res = mysql_store_result(my);
            int rows = mysql_num_rows(res);
            int cols = mysql_num_fields(res);
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
    public:
        bool GetAllQuestions(std::vector<Question>* qs)
        {
            std::string sql = "select * from ";
            sql += oj_questions;
            return QueryMySql(sql, qs);
        }
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

    };
};