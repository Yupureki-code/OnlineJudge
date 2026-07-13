#pragma once
#include "handler_base.hpp"
#include <string>
#include <fstream>
#include <chrono>
#include <atomic>
#include <sys/stat.h>

namespace oj_judge {

/// 预处理器：将用户代码写入临时 .cpp 文件
class HandlerPreProcesser : public HandlerProgram 
{
public:
    std::string Execute(const std::string& file_name, const oj_judge::SubmitRequest& request,std::coroutine_handle<> h) override 
    {
        // 生成唯一文件名
        std::string name = GetUniqueFileName();
        // 写入源文件
        std::string src_path = SrcPath(name);
        _file_system.write(src_path, request.code());

        // 传递文件名给下一个处理器
        if (_next) {
            return _next->Execute(name, request,h);
        }
        //return HandlerEnd({UNKNOWN}, name, request);
    }

private:
    /// 临时文件目录
    static constexpr const char* TMP_DIR = "../tmp/";

    std::string SrcPath(const std::string& name) const {
        return std::string(TMP_DIR) + name + ".cpp";
    }

    std::string GetUniqueFileName() const {
        static std::atomic<uint64_t> id{0};
        auto now = std::chrono::steady_clock::now().time_since_epoch();
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
        return std::to_string(ms) + "_" + std::to_string(id.fetch_add(1));
    }

    std::string HandlerEnd(const std::vector<Status>& result,
                           const std::string& file_name,
                           const std::string& in_json) {
        Json::Value out;
        out["status"] = StatusToString(result.empty() ? UNKNOWN : result[0]);
        out["desc"] = StatusToDesc(result.empty() ? UNKNOWN : result[0]);
        Json::StyledWriter writer;
        return writer.write(out);
    }
};

} // namespace oj_judge