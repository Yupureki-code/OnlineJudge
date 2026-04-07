#pragma once

#include <cstdio>
#include <cstdlib>
#include <ctemplate/template_string.h>
#include <jsoncpp/json/reader.h>
#include <jsoncpp/json/value.h>
#include <string>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <fcntl.h>
#include "../../comm/comm.hpp"
#include "../../comm/logstrategy.hpp"
#include "COP_hanlder.hpp"
#include <mysql/mysql.h>

// 运行类:运行exe文件，从stdin中获得输入数据。
// 1.若发生运行中错误，向stderr写入错误类型
// 2.若正确，向stdout写入输出内容

namespace ns_runner
{
    using namespace ns_hanlder;
    using namespace ns_log;
    using namespace ns_util;

    const std::string oj_tests = "tests";
    const std::string oj_questions = "questions";
    const std::string host = "127.0.0.1";
    const std::string user = "oj_server";
    const std::string passwd = "Myoj@localhost2026071024";
    const std::string db = "myoj";
    const int port = 3306;

    struct Test
    {
        std::string in;
        std::string out;
        int cpu_limit;
        int mem_limit;
    };

    class HandlerRunner : public HandlerProgram
    {
    private:
        //设置时间和内存限制
        void SetProcLimit(int cpu_limit,int mem_limit)
        {
            logger(ns_log::DEBUG)<<"设置资源限制 - CPU: "<<cpu_limit<<"秒, 内存: "<<mem_limit<<"字节";
            
            struct rlimit r;
            // 设置CPU时间限制
            r.rlim_cur = cpu_limit;
            r.rlim_max = RLIM_INFINITY;
            if(setrlimit(RLIMIT_CPU, &r) < 0) {
                logger(ns_log::ERROR)<<"设置CPU限制失败";
            }
            
            // 设置内存限制 - 使用RLIMIT_AS限制地址空间
            r.rlim_cur = mem_limit;
            r.rlim_max = mem_limit;
            if(setrlimit(RLIMIT_AS, &r) < 0) {
                logger(ns_log::ERROR)<<"设置内存限制失败";
            }
        }
        //查询Mysql，获得题目输入,输出内容，时间限制和内存限制
        bool QueryMysql(const std::string& question_id,std::vector<Test>* tests)
        {
            MYSQL* my = mysql_init(nullptr);
            if(mysql_real_connect(my, host.c_str(), user.c_str(), passwd.c_str(), db.c_str(), port, nullptr, 0) == nullptr)
            {
                logger(ns_log::FATAL)<<"MySQL连接失败: "<<mysql_error(my);
                return false;
            }
            //查询时间和空间限制
            std::string question_sql = "select cpu_limit,mem_limit from " + oj_questions  + " where id='"+question_id+"'"; 
            if(mysql_query(my, question_sql.c_str()) != 0)
            {
                logger(ns_log::FATAL)<<"MySql查询错误: "<<mysql_error(my);
                return false;
            }
            MYSQL_RES* res = mysql_store_result(my);
            if(res == nullptr)
            {
                logger(ns_log::FATAL)<<"MySQL获取结果集失败: "<<mysql_error(my);
                mysql_close(my);
                return false;
            }
            MYSQL_ROW row = mysql_fetch_row(res);
            if(row == nullptr)
            {
                logger(ns_log::FATAL)<<"MySQL获取行失败: "<<mysql_error(my);
                mysql_free_result(res);
                mysql_close(my);
                return false;
            }
            int cpu_limit = atoi(row[0]);
            int mem_limit = atoi(row[1]);
            mysql_free_result(res);
            //查询输入，输出内容
            std::string tests_sql = "select `in`,`out` from " + oj_tests + " where question_id='" + question_id + "'";
            if(mysql_query(my, tests_sql.c_str()) != 0)
            {
                logger(ns_log::FATAL)<<"MySql查询错误: "<<mysql_error(my);
                return false;
            }
            res = mysql_store_result(my);
            if(res == nullptr)
            {
                logger(ns_log::FATAL)<<"MySQL获取结果集失败: "<<mysql_error(my);
                mysql_close(my);
                return false;
            }
            int rows = mysql_num_rows(res);
            for(int i = 0;i<rows;i++)
            {
                Test test;
                MYSQL_ROW row = mysql_fetch_row(res);
                if(row == nullptr)
                {
                    logger(ns_log::FATAL)<<"MySQL获取行失败: "<<mysql_error(my);
                    mysql_free_result(res);
                    mysql_close(my);
                    return false;
                }
                test.in = row[0];
                test.out = row[1];
                test.cpu_limit = cpu_limit;
                test.mem_limit = mem_limit;
                tests->push_back(test);
            }
            mysql_free_result(res);
            mysql_close(my);
            return true;
        }
    public:
        std::string Execute(const std::string& file_name,const std::string& in_json)override
        {
            if(_is_enable)
            {   
                std::string _run_file = PathUtil::Exe(file_name);
                umask(0);
                pid_t pid = fork();
                if(pid < 0)
                {
                    logger(ns_log::FATAL)<<"创建子进程失败";
                    return HandlerProgramEnd({UNKNOWN}, file_name);
                }
                else if(pid == 0)
                {
                    Json::Value in_value;
                    Json::Reader reader;
                    reader.parse(in_json,in_value);
                    std::vector<Test> tests;
                    if(!QueryMysql(in_value["id"].asString(), &tests))
                    {
                        logger(FATAL)<<"MySQL查询题目"<<in_value["id"].asString()<<" 失败!";
                        exit(1);
                    }
                    //测试用例id
                    int test_number = 0;
                    //多个测试用例，分别处理
                    for(auto & test : tests)
                    {
                        test_number++;
                        std::string test_file = file_name + "_"+std::to_string(test_number);
                        std::string _stdin  = PathUtil::Stdin(test_file);
                        std::string _stdout = PathUtil::Stdout(test_file);
                        std::string _stderr = PathUtil::Stderr(test_file);
                        std::string _ans = PathUtil::Ans(test_file);
                        int _stdin_fd  = open(_stdin.c_str(),O_CREAT | O_RDONLY,0644);
                        int _stdout_fd = open(_stdout.c_str(),O_CREAT | O_WRONLY,0644);
                        int _stderr_fd = open(_stderr.c_str(),O_CREAT | O_WRONLY,0644);
                        int _ans_fd = open(_ans.c_str(),O_CREAT | O_WRONLY,0644);
                        if(!FileUtil::WriteFile(_stdin, test.in) || !FileUtil::WriteFile(_ans, test.out))
                        {
                            logger(FATAL)<<"写入 "<<_stdin <<" || "<<_ans<<" 错误!";
                            exit(1);
                        }
                        if(_stdin_fd < 0 || _stdout_fd < 0 || _stderr_fd < 0 || _ans_fd < 0)
                        {
                            logger(ns_log::FATAL)<<"打开_stdin _stdout _stderr _ans文件失败";
                            exit(1);
                        }
                        close(_ans_fd);
                        pid_t pid = fork();
                        if(pid < 0)
                        {
                            logger(ns_log::FATAL)<<"创建子进程失败";
                            exit(1);
                        }
                        else if(pid == 0)
                        {
                            //子进程执行程序，重定向文件描述符
                            SetProcLimit(test.cpu_limit, test.mem_limit);
                            dup2(_stdin_fd,0);
                            dup2(_stdout_fd,1);
                            dup2(_stderr_fd,2);
                            execl(_run_file.c_str(), _run_file.c_str(),nullptr);
                            _exit(1);
                        }
                        else
                        {
                            //父进程等待子进程执行，获得状态码
                            int wait_status = 0;
                            waitpid(pid,&wait_status,0);
                            logger(ns_log::DEBUG)<<test_number<<" 的status:"<<wait_status;
                            //根据状态码判断，若是运行中错误直接返回
                            if(WIFEXITED(wait_status))
                            {
                                int exit_code = WEXITSTATUS(wait_status);
                                if(exit_code != 0)
                                    FileUtil::WriteFile(_stderr, StatusToDesc(ExitCodeToSatusCode(exit_code)));
                            }
                            else if(WIFSIGNALED(wait_status))
                            {
                                int signal = WTERMSIG(wait_status);
                                FileUtil::WriteFile(_stderr, StatusToDesc(ExitCodeToSatusCode(signal)));
                            }
                            close(_stdin_fd);
                            close(_stdout_fd);
                            close(_stderr_fd);
                        }
                    }
                    exit(0);
                }
                else 
                {
                    //父进程获得子进程退出码
                    int status = 0;
                    waitpid(pid,&status,0);
                    // 检查进程是否正常退出
                    if (WIFEXITED(status)) 
                    {
                        int exit_code = WEXITSTATUS(status);
                        logger(ns_log::INFO)<<"运行完毕,ExitCode: "<<exit_code;
                        if(exit_code != 0)
                            return HandlerProgramEnd({ExitCodeToSatusCode(exit_code)}, file_name);
                    } 
                    else if (WIFSIGNALED(status)) 
                    {
                        // 进程被信号终止
                        int signal = WTERMSIG(status);
                        logger(ns_log::INFO)<<"运行被信号终止,Signal: "<<signal;
                        return HandlerProgramEnd({ExitCodeToSatusCode(signal)}, file_name);
                    } 
                    else 
                    {
                        // 其他情况
                        logger(ns_log::INFO)<<"运行状态未知,Status: "<<status;
                        return HandlerProgramEnd({UNKNOWN}, file_name);
                    }
                }
            }
            // 运行成功，调用下一个处理器（judge）
            if(_next)
            {
                return _next->Execute(file_name,in_json);
            }
            else 
            {
                //责任链不应该在这结束
                ns_log::logger(ns_log::INFO)<<"责任链结束";
                return HandlerProgramEnd({UNKNOWN}, file_name);
            }
        }
    };
};
