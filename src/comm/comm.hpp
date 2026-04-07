#pragma once

#include "config.h"
#include <string>
#include <atomic>
#include <fstream>
#include <vector>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/time.h>
#include <signal.h>
#include <cstdlib>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>

const char *root = "/";
const char *dev_null = "/dev/null";

//设置守护进程
void Daemon(bool ischdir, bool isclose)
{
    signal(SIGCHLD, SIG_IGN);
    signal(SIGPIPE, SIG_IGN);
    if (fork() > 0)
    exit(0);
    setsid();
    if (ischdir)
    chdir(root);
    if (isclose)
    {
        close(0);
        close(2);
    }
    else
    {
        int fd = open(dev_null, O_RDWR);
        if (fd > 0)
        {
            dup2(fd, 0);
            dup2(fd, 2);
            close(fd);
        }
    }
}

//工具类
namespace ns_util
{
    const std::string default_path = "../tmp/";
    //时间工具
    class TimeUtil
    {
    public:
        //获取时间戳
        static std::string GetTimeStamp()
        {
            struct timeval _time;
            gettimeofday(&_time, nullptr);
            return std::to_string(_time.tv_sec);
        }
        //获取毫秒级别时间戳
        static std::string GetTimeMs()
        {
            struct timeval _time;
            gettimeofday(&_time, nullptr);
            return std::to_string(_time.tv_sec * 1000 + _time.tv_usec / 1000);
        }
    };
    //路径工具
    class PathUtil
    {
    public:
        //添加后缀
        static std::string AddSuffix(const std::string& file_name,const std::string& suffix)
        {
            return file_name + suffix;
        }
        static std::string Src(const std::string& file_name)
        {
            return default_path + AddSuffix(file_name,".cpp");
        }
        static std::string Exe(const std::string& file_name)
        {
            return default_path + AddSuffix(file_name,".exe");
        }
        static std::string Stdin(const std::string & file_name)
        {
            return AddSuffix(file_name, ".txt");
        }
        static std::string Stdout(const std::string & file_name)
        {
            return default_path + AddSuffix(file_name, ".stdout");
        }
        static std::string Compile_err(const std::string & file_name)
        {
            return default_path + AddSuffix(file_name, ".compile_err");
        }
        static std::string Stderr(const std::string& file_name)
        {
            return default_path + AddSuffix(file_name, ".stderr");
        }
        static std::string Ans(const std::string& file_name)
        {
            return default_path + AddSuffix(file_name, ".ans");
        }
    };
    //文件工具
    class FileUtil
    {
    public:
        //判断文件是否存在
        static bool IsFileExist(const std::string& path_name)
        {
            struct stat st;
            if(stat(path_name.c_str(),&st) == 0)
                return true;
            return false;
        }
        //获得一个唯一的文件名字
        static std::string GetUniqueFileName()
        {
             static std::atomic_uint id(0);
            id++;
            std::string ms = TimeUtil::GetTimeMs();
            std::string uniq_id = std::to_string(id);
            return ms + "_" + uniq_id;
        }
        //向文件中写入
        static bool WriteFile(const std::string &target, const std::string &content)
        {
            std::ofstream out(target);
            if (!out.is_open())
            {
                return false;
            }
            out.write(content.c_str(), content.size());
            out.close();
            return true;
        }
        //从文件中读取
        static bool ReadFile(const std::string &target, std::string *content, bool keep = false)
        {
            (*content).clear();

            std::ifstream in(target);
            if (!in.is_open())
            {
                return false;
            }
            std::string line;
            while (std::getline(in, line))
            {
                (*content) += line;
                (*content) += (keep ? "\n" : "");
            }
            in.close();
            return true;
        }
        //移除临时文件
        static void RemoveTmpFiles(const std::string& file_name)
        {
            std::string _src = PathUtil::Src(file_name);
            if(FileUtil::IsFileExist(_src)) unlink(_src.c_str());

            std::string _compiler_error = PathUtil::Compile_err(file_name);
            if(FileUtil::IsFileExist(_compiler_error)) unlink(_compiler_error.c_str());

            std::string _execute = PathUtil::Exe(file_name);
            if(FileUtil::IsFileExist(_execute)) unlink(_execute.c_str());
            for(int i = 1;;i++)
            {
                std::string test_file = file_name + "_" + std::to_string(i);
                std::string _stdin = PathUtil::Stdin(test_file);
                if(FileUtil::IsFileExist(_stdin)) unlink(_stdin.c_str());
                else break;
                std::string _stderr = PathUtil::Stderr(test_file);
                if(FileUtil::IsFileExist(_stderr)) unlink(_stderr.c_str());
                else break;
                std::string _stdout = PathUtil::Stdout(test_file);
                if(FileUtil::IsFileExist(_stdout)) unlink(_stdout.c_str());
                else break;
                std::string _ans = PathUtil::Ans(test_file);
                if(FileUtil::IsFileExist(_ans)) unlink(_ans.c_str());
                else break;
            }
        }
    };
    //字符串工具
    class StringUtil
    {
    public:
        //从字符串中根据分隔符，获得每部分的字符串
        static void SplitString(std::string str,std::string space,std::vector<std::string>* v)
        {
            size_t pos = 0;
            while ((pos = str.find(space)) != std::string::npos)
            {
                std::string token = str.substr(0, pos);
                if (!token.empty())
                {
                    v->push_back(token);
                }
                str.erase(0, pos + space.length());
            }
            if (!str.empty())
            {
                v->push_back(str);
            }
        }
        //移除空格，换行符等
         static std::string RemoveAllWhitespace(const std::string& str)
         {
            std::string result;
            for (char c : str)
            {
                if (c != ' ' && c != '\n' && c != '\r' && c != '\t')
                {
                    result += c;
                }
            }
            return result;
         }
    };
    class ConfigUtil
    {

    };
};