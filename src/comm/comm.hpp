#pragma once

#include <jsoncpp/json/json.h>
#include <chrono>
#include <sys/types.h>
#include <sys/stat.h>
#include <memory>
#include <string>
#include <atomic>
#include <sys/time.h>
#include <any>
#include <signal.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <regex>
#include <chrono>
#include "config.h"
#include "models/types.hxx"
#include <cassert>
#include <iomanip>

inline constexpr const char root[] = "/";
inline constexpr const char dev_null[] = "/dev/null";

namespace OnlineJudge
{
    struct Response 
    {
        bool status = true;         // 0 表示成功，负值表示错误
        std::string errmsg;  // 错误信息
        std::any value;      // 返回的任意数据
        
        Response() : status(true) {}
        Response(int s, const std::string& msg = "") : status(s), errmsg(msg) {}
    };
}

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
    int id = 0;
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

enum class CommentStatus
{
    normal,
    hidden
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
inline void Daemon(bool ischdir, bool isclose)
{
    signal(SIGCHLD, SIG_IGN);
    signal(SIGPIPE, SIG_IGN);
    if (fork() > 0)
    {
        exit(0);
    }
    setsid();
    if (ischdir)
    {
        chdir(root);
    }
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
namespace oj::util
{
    const std::string default_path = "../tmp/";
    //时间工具
    class TimeUtil
    {
    public:
        // 获取与 DateTimeToInt/IntToDateTime 一致的微秒时间戳。
        static inline int64_t GetTimeStamp()
        {
            auto now = std::chrono::system_clock::now();
            auto us = std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch());
            return us.count();
        }
        //获取毫秒级别时间戳
        static std::string GetTimeMs()
        {
            struct timeval _time;
            gettimeofday(&_time, nullptr);
            return std::to_string(_time.tv_sec * 1000 + _time.tv_usec / 1000);
        }
        static inline int64_t DateTimeToInt(const MYSQL_TIME& t) noexcept 
        {
            if (t.year < 1970 || t.month < 1 || t.month > 12 || t.day < 1 || t.day > 31 ||
                t.hour > 23 || t.minute > 59 || t.second > 59 || t.second_part > 999999)
                return 0;

            std::tm value{};
            value.tm_year = static_cast<int>(t.year) - 1900;
            value.tm_mon = static_cast<int>(t.month) - 1;
            value.tm_mday = static_cast<int>(t.day);
            value.tm_hour = static_cast<int>(t.hour);
            value.tm_min = static_cast<int>(t.minute);
            value.tm_sec = static_cast<int>(t.second);
            const std::time_t seconds = timegm(&value);
            if (seconds < 0 || value.tm_year != static_cast<int>(t.year) - 1900 ||
                value.tm_mon != static_cast<int>(t.month) - 1 ||
                value.tm_mday != static_cast<int>(t.day) ||
                value.tm_hour != static_cast<int>(t.hour) ||
                value.tm_min != static_cast<int>(t.minute) ||
                value.tm_sec != static_cast<int>(t.second))
                return 0;
            return static_cast<int64_t>(seconds) * 1000000 + t.second_part;
        }
        static inline MYSQL_TIME IntToDateTime(int64_t ts_us) noexcept 
        {
            MYSQL_TIME t{};
            int64_t seconds = ts_us / 1000000;
            int64_t micros = ts_us % 1000000;
            if (micros < 0) {
                micros += 1000000;
                --seconds;
            }
            const std::time_t value = static_cast<std::time_t>(seconds);
            std::tm utc{};
            if (gmtime_r(&value, &utc) == nullptr)
                return t;
            t.year        = utc.tm_year + 1900;
            t.month       = utc.tm_mon + 1;
            t.day         = utc.tm_mday;
            t.hour        = utc.tm_hour;
            t.minute      = utc.tm_min;
            t.second      = utc.tm_sec;
            t.second_part = static_cast<unsigned long>(micros);
            t.time_type   = MYSQL_TIMESTAMP_DATETIME;
            return t;
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
    //字符串工具
    class StringUtil
    {
    public:
        static std::string GetUniqueName()
        {
            static std::atomic_uint id(0);
            id++;
            std::string ms = TimeUtil::GetTimeMs();
            std::string uniq_id = std::to_string(id);
            return ms + "_" + uniq_id;
        } 
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
        //去除全部的空格
        static std::string TrimSpace(const std::string& input)
        {
            std::string out = input;
            out.erase(out.begin(), std::find_if(out.begin(), out.end(), [](unsigned char ch){ return !std::isspace(ch); }));
            out.erase(std::find_if(out.rbegin(), out.rend(), [](unsigned char ch){ return !std::isspace(ch); }).base(), out.end());
            return out;
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

    class HttpUtil
    {
    public:
        static std::string url_encode(const std::string& value)
        {
            std::ostringstream encoded;
            encoded << std::uppercase << std::hex;
            for (unsigned char ch : value)
            {
                if (std::isalnum(ch) || ch == '-' || ch == '_' || ch == '.' || ch == '~')
                {
                    encoded << static_cast<char>(ch);
                }
                else
                {
                    encoded << '%' << std::setw(2) << std::setfill('0') << static_cast<int>(ch);
                }
            }
            return encoded.str();
        }
    };
}
