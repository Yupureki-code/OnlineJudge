#pragma once

#include "config.h"
#include <httplib.h>
#include <memory>
#include <string>
#include <atomic>
#include <fstream>
#include <utility>
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
#include <jsoncpp/json/json.h>

using namespace httplib;

const char *root = "/";
const char *dev_null = "/dev/null";

struct User
{
    int uid; //用户ID
    std::string name; //用户名
    std::string email; //邮箱
    std::string create_time; //创建时间
    std::string last_login; //最后登录时间
    std::string password_algo; //密码算法
};

struct AdminAccount
{
    int admin_id = 0;
    std::string password_hash;
    int uid = 0;
    std::string role;
    std::string created_at;
};

struct AdminAuditLog
{
    std::string request_id;
    int operator_admin_id = 0;
    int operator_uid = 0;
    std::string operator_role;
    std::string action;
    std::string resource_type;
    std::string result;
    std::string payload_text;
};

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

struct QueryStruct
{
    virtual std::string ToString() const = 0;
    virtual std::string ToJsonString() const = 0;
};

struct QuestionQuery : public QueryStruct
{
    std::string id;
    std::string title;
    std::string star;
    std::string ToString() const override
    {
        return "id:" + id + ",title:" + title + ",star:" + star;
    }
    std::string ToJsonString() const override
    {
        Json::Value json_value;
        json_value["id"] = id;
        json_value["title"] = title;
        json_value["star"] = star;
        Json::FastWriter writer;
        return writer.write(json_value);
    }
};

struct UserQuery : public QueryStruct
{
    int id;
    std::string name;
    std::string email;
    std::string ToString() const override
    {
        return "id:" + std::to_string(id) + ",name:" + name + ",email:" + email;
    }
    std::string ToJsonString() const override
    {
        Json::Value json_value;
        json_value["id"] = id;
        json_value["name"] = name;
        json_value["email"] = email;
        Json::FastWriter writer;
        return writer.write(json_value);
    }
};



enum class SolutionStatus
{
    pending,
    approved,
    rejected
};

enum class SolutionSort
{
    latest,
    hot
};

enum class SolutionActionType
{
    none,
    like,
    favorite
};

enum class ListType
{
    Questions,
    Users
};

class EnumToStringUtil
{
public:
    static std::string ListToString(ListType type)
    {
        switch (type)
        {
            case ListType::Questions:
                return "question";
            case ListType::Users:
                return "user";
            default:
                return "unknown";
        }
        }
};

struct KeyContext
{
    // 现有题库列表缓存字段（保留）
    std::shared_ptr<QueryStruct> _query;
    int page = 1;
    int size = 10;

    ListType list_type = ListType::Questions;
    std::string list_version;
    std::string _page_name;

    // solution 专用字段（新增）
    std::string question_id;      // 题号或题目主键，与数据库保持一致
    long long solution_id = 0;    // 题解ID
    int user_id = 0;              // 当前用户ID（资格/互动状态用）
    SolutionStatus status = SolutionStatus::approved;
    SolutionSort sort = SolutionSort::latest;
    SolutionActionType action_type = SolutionActionType::none;
};

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
        //去除字符串首尾的空格
        static std::string Trim(const std::string& s)
        {
            size_t start = 0;
            while (start < s.size() && std::isspace(static_cast<unsigned char>(s[start])))
                ++start;
            size_t end = s.size();
            while (end > start && std::isspace(static_cast<unsigned char>(s[end - 1])))
                --end;
            return s.substr(start, end - start);
        }
        //判断邮箱是否合法
        static bool IsValidEmail(const std::string& email)
        {
            static const std::regex kEmailPattern(R"(^[A-Za-z0-9._%+\-]+@[A-Za-z0-9.\-]+\.[A-Za-z]{2,}$)");
            return std::regex_match(email, kEmailPattern);
        }
        //判断pagename是否合法
        static bool IsSafeStaticPageName(const std::string& page_name)
        {
            if (page_name.empty())
            {
                return false;
            }
            if (page_name.find("..") != std::string::npos)
            {
                return false;
            }
            if (page_name.front() == '/' || page_name.front() == '\\')
            {
                return false;
            }
            return true;
        }
        //判断验证码是否合法
        static bool IsValidAuthCode(const std::string& code)
        {
            if (code.size() != 6)
            {
                return false;
            }
            return std::all_of(code.begin(), code.end(), [](unsigned char ch){ return std::isdigit(ch); });
        }
    };
    class JsonUtil
    {
    public:
        //从请求体中解析出json
        static bool ParseJsonBody(const Request& req, Json::Value* out)
        {
            if (out == nullptr || req.body.empty())
            {
                return false;
            }

            Json::CharReaderBuilder builder;
            std::string errs;
            std::istringstream ss(req.body);
            return Json::parseFromStream(builder, ss, out, &errs);
        }
        //从Json中提取email，并验证格式是否合法
        static bool ExtractAndValidateEmail(const Json::Value& in_value, std::string* email)
        {
            if (email == nullptr || !in_value.isMember("email") || !in_value["email"].isString())
            {
                return false;
            }

            *email = StringUtil::Trim(in_value["email"].asString());
            if (email->empty() || !StringUtil::IsValidEmail(*email))
            {
                return false;
            }
            return true;
        }
        //从Json中提取password，并验证格式是否合法
        static bool ExtractAndValidatePassword(const Json::Value& in_value, std::string* password)
        {
            if (password == nullptr || !in_value.isMember("password") || !in_value["password"].isString())
            {
                return false;
            }

            *password = in_value["password"].asString();
            if (password->empty())
            {
                return false;
            }
            return true;
        }

    };
    class HttpUtil
    {
    public:
        //对字符串进行URL编码
        static std::string url_encode(const std::string &value) 
        {
            std::string result;
            result.reserve(value.size());
            for (char c : value) 
            {
                if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') 
                {
                    result += c;
                } 
                else 
                {
                    result += '%';
                    char hex[3];
                    snprintf(hex, sizeof(hex), "%02X", static_cast<unsigned char>(c));
                    result += hex;
                }
            }
            return result;
        }
        //回复400错误的函数，返回一个JSON格式的错误信息
        static void ReplyBadRequest(Response& rep, const std::string& message)
        {
            Json::Value response;
            response["success"] = false;
            response["error"] = message;
            Json::FastWriter writer;
            rep.status = 400;
            rep.set_content(writer.write(response), "application/json;charset=utf-8");
        }
        //从请求中解析正整数参数
        static int ParsePositiveIntParam(const Request& req, const std::string& key, int default_value, int min_value, int max_value)
        {
            if (!req.has_param(key))
            {
                return default_value;
            }

            try
            {
                int value = std::stoi(req.get_param_value(key));
                if (value < min_value) return min_value;
                if (value > max_value) return max_value;
                return value;
            }
            catch (...)
            {
                return default_value;
            }
        }
    };
};