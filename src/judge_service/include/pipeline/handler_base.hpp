#pragma once
#include <string>
#include <vector>
#include <memory>
#include "../../../comm/filesystem.hpp"
#include "../../../comm/proto/judge_service.pb.h"
#include <jsoncpp/json/value.h>
#include <jsoncpp/json/writer.h>
#include <jsoncpp/json/reader.h>
#include <coroutine>

namespace oj_judge {

/// 判题状态码
enum Status {
    AC = 0,           // 通过
    WA,               // 答案错误
    RUNTIME_ERROR,    // 运行时错误
    MEMORY_LIMIT,     // 内存超限
    COMPILE_ERROR,    // 编译错误
    TIME_LIMIT,       // 时间超限
    FPE_ERROR,        // 浮点异常
    SEGV_ERROR,       // 段错误
    UNKNOWN           // 未知错误
};

/// 判题最终结果
struct JudgeFinalResult {
    Status overall_status;     // 总体状态
    std::vector<TestCaseResult> test_cases;  // 各测试用例结果
    int total_time_ms;         // 总耗时
    int64_t max_memory_bytes;  // 最大内存
    std::string compile_error; // 编译错误信息
    std::string stdout_str;    // 标准输出（自定义测试用）
    std::string stderr_str;    // 标准错误（自定义测试用）
    bool is_custom_test;       // 是否自定义测试
};

/// 退出码转状态码
inline Status ExitCodeToStatusCode(int code) {
    switch (code) {
        case 0:   return AC;
        case -1:  return WA;
        case 8:   return FPE_ERROR;       // SIGFPE
        case 136: return FPE_ERROR;       // exit code for SIGFPE
        case 11:  return SEGV_ERROR;      // SIGSEGV
        case 139: return SEGV_ERROR;      // exit code for SIGSEGV
        case 6:   return RUNTIME_ERROR;   // SIGABRT
        case 134: return RUNTIME_ERROR;   // exit code for SIGABRT
        case 24:  return TIME_LIMIT;      // SIGXCPU
        case 152: return TIME_LIMIT;      // exit code for SIGXCPU
        default:  return UNKNOWN;
    }
}

/// 状态码转描述
inline std::string StatusToDesc(Status status) {
    switch (status) {
        case AC:             return "恭喜你!已通过此题";
        case WA:             return "部分解答错误";
        case RUNTIME_ERROR:  return "运行时错误";
        case MEMORY_LIMIT:   return "内存超限";
        case COMPILE_ERROR:  return "编译错误";
        case TIME_LIMIT:     return "运行时间超时";
        case FPE_ERROR:      return "浮点数异常";
        case SEGV_ERROR:     return "段错误";
        case UNKNOWN:        return "服务器繁忙，请稍后重试";
        default:             return "未知错误";
    }
}

/// 状态码转字符串
inline std::string StatusToString(Status status) {
    switch (status) {
        case AC:             return "AC";
        case WA:             return "WA";
        case RUNTIME_ERROR:  return "RTE";
        case MEMORY_LIMIT:   return "MLE";
        case COMPILE_ERROR:  return "CE";
        case TIME_LIMIT:     return "TLE";
        case FPE_ERROR:      return "FPE";
        case SEGV_ERROR:     return "SEGV";
        case UNKNOWN:        return "UNKNOWN";
        default:             return "UNKNOWN";
    }
}

/// 链式处理器基类
///
/// 责任链模式：preprocesser → compiler → runner → judge
/// 每个 Handler 接收 file_name 和 in_json，处理后调用 _next
class HandlerProgram {
public:
    virtual ~HandlerProgram() = default;

    /// 执行处理
    /// @param file_name 临时文件名（无后缀）
    /// @param in_json 输入 JSON
    /// @return 判题结果 JSON
    virtual std::string Execute(const std::string& file_name, const oj_judge::SubmitRequest& request,std::coroutine_handle<> h) = 0;

    /// 设置下一个处理器
    void SetNext(std::shared_ptr<HandlerProgram> next) { _next = std::move(next); }

    void Enable()  { _is_enable = true; }
    void Disable() { _is_enable = false; }

protected:
    std::shared_ptr<HandlerProgram> _next;
    fileUtil::FileSystem _file_system;
    bool _is_enable = true;
};

} // namespace oj_judge