#pragma once

#include <cstdio>
#include <assert.h>
#include <cstdlib>
#include <vector>
#include "../comm/logstrategy.hpp"
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

    const std::string oj_questions = "questions";
    const std::string host = "127.0.0.1";
    const std::string user = "oj_server";
    const std::string passwd = "Myoj@localhost2026071024";
    const std::string db = "myoj";
    const int port = 3306;

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
                logger(ns_log::FATAL)<<"MySql查询错误!";
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

    };
};