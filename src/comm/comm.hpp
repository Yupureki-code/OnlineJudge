#pragma once

#include "config.h"
#include "logger.hpp"
#include <jsoncpp/json/json.h>
#include <chrono>
#include <condition_variable>
#include <deque>
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

struct Solution
{
    unsigned long long id = 0; //题解ID
    std::string question_id;   //题号
    int user_id = 0;           //作者UID
    std::string title;         //题解标题
    std::string content_md;    //Markdown内容
    unsigned int like_count = 0;
    unsigned int favorite_count = 0;
    unsigned int comment_count = 0;
    SolutionStatus status = SolutionStatus::approved;
    std::string created_at;
    std::string updated_at;
};

struct Comment
{
    unsigned long long id = 0;
    unsigned long long solution_id = 0;
    int user_id = 0;
    std::string content;
    bool is_edited = false;
    unsigned long long parent_id = 0;           // 0表示顶级评论，非0表示回复
    int reply_to_user_id = 0;                   // 被回复用户ID，用于@引用
    unsigned int like_count = 0;                // 点赞数
    unsigned int favorite_count = 0;            // 收藏数
    std::string reply_to_user_name = "";      // 被回复用户名（查询时填充）
    std::string author_name = "";             // 评论作者用户名（查询时通过JOIN填充）
    std::string created_at;
    std::string updated_at;
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
namespace oj_util
{
    const std::string default_path = "../tmp/";
    //时间工具
    class TimeUtil
    {
    public:
        //获取时间戳
        static inline int64_t GetTimeStamp()
        {
            auto now = std::chrono::system_clock::now();
            auto ms  = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch());
            return ms.count();
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
}
