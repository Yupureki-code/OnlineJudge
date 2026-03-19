#pragma once

#include <string>
#include <atomic>
#include <fstream>
#include <vector>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/time.h>

namespace ns_util
{
    const std::string default_path = "../tmp/";
    class TimeUtil
    {
    public:
        static std::string GetTimeStamp()
        {
            struct timeval _time;
            gettimeofday(&_time, nullptr);
            return std::to_string(_time.tv_sec);
        }
        static std::string GetTimeMs()
        {
            struct timeval _time;
            gettimeofday(&_time, nullptr);
            return std::to_string(_time.tv_sec * 1000 + _time.tv_usec / 1000);
        }
    };
    class PathUtil
    {
    public:
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
    };

    class FileUtil
    {
    public:
        static bool IsFileExist(const std::string& path_name)
        {
            struct stat st;
            if(stat(path_name.c_str(),&st) == 0)
                return true;
            return false;
        }
        static std::string GetUniqueFileName()
        {
             static std::atomic_uint id(0);
            id++;
            std::string ms = TimeUtil::GetTimeMs();
            std::string uniq_id = std::to_string(id);
            return ms + "_" + uniq_id;
        }
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
        static void RemoveTmpFiles(const std::string& file_name)
        {
            std::string _src = PathUtil::Src(file_name);
            if(FileUtil::IsFileExist(_src)) unlink(_src.c_str());

            std::string _compiler_error = PathUtil::Compile_err(file_name);
            if(FileUtil::IsFileExist(_compiler_error)) unlink(_compiler_error.c_str());

            // std::string _execute = PathUtil::Exe(file_name);
            // if(FileUtil::IsFileExist(_execute)) unlink(_execute.c_str());

            std::string _stdin = PathUtil::Stdin(file_name);
            if(FileUtil::IsFileExist(_stdin)) unlink(_stdin.c_str());

            std::string _stdout = PathUtil::Stdout(file_name);
            if(FileUtil::IsFileExist(_stdout)) unlink(_stdout.c_str());

            std::string _stderr = PathUtil::Stderr(file_name);
            if(FileUtil::IsFileExist(_stderr)) unlink(_stderr.c_str());
        }
    };

    class StringUtil
    {
    public:
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
    };
};