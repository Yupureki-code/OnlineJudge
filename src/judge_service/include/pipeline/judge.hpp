#pragma once
#include "handler_base.hpp"
#include <string>
#include <fstream>
#include <sys/stat.h>

namespace oj_judge {

/// 判题器：比较用户输出与期望输出
///
/// 读取每个测试用例的 stdout 和 stderr：
/// - stderr 非空 → 运行时错误（TLE/MLE/FPE/SEGV）
/// - stdout 与 ans 匹配 → AC，否则 WA
class HandlerJudge : public HandlerProgram {
public:
    std::string Execute(const std::string& file_name, const oj_judge::SubmitRequest& request,std::coroutine_handle<> h) override {
        std::vector<Status> results;

        if (!_is_enable) {
            return _next ? _next->Execute(file_name, request,h) : HandlerEnd(results, file_name);
        }

        // 逐个测试用例判断
        for (int i = 1;; ++i) {
            std::string test_file = file_name + "_" + std::to_string(i);
            std::string stderr_path = TmpPath(test_file, ".stderr");

            // 检查 stderr 文件是否存在
            struct stat st;
            if (stat(stderr_path.c_str(), &st) != 0) {
                // 文件不存在，说明所有测试用例已处理完
                if (!results.empty()) break;
                results.push_back(UNKNOWN);
                break;
            }

            // 读取 stderr
            std::string err;
            ReadFile(stderr_path, err);

            if (!err.empty()) {
                // 有运行时错误
                if (err.find("运行时间超时") != std::string::npos)
                    results.push_back(TIME_LIMIT);
                else if (err.find("内存超限") != std::string::npos ||
                         err.find("内存申请过多") != std::string::npos)
                    results.push_back(MEMORY_LIMIT);
                else if (err.find("浮点数异常") != std::string::npos)
                    results.push_back(FPE_ERROR);
                else if (err.find("段错误") != std::string::npos)
                    results.push_back(SEGV_ERROR);
                else if (err.find("运行时错误") != std::string::npos)
                    results.push_back(RUNTIME_ERROR);
                else
                    results.push_back(UNKNOWN);
            } else {
                // 运行正常，比较输出
                std::string stdout_path = TmpPath(test_file, ".stdout");
                std::string ans_path = TmpPath(test_file, ".ans");

                std::string user_out, expected;
                ReadFile(stdout_path, user_out);
                ReadFile(ans_path, expected);

                if (RemoveAllWhitespace(user_out) == RemoveAllWhitespace(expected)) {
                    results.push_back(AC);
                } else {
                    results.push_back(WA);
                }
            }
        }

        return HandlerEnd(results, file_name);
    }

private:
    static constexpr const char* TMP_DIR = "../tmp/";

    std::string TmpPath(const std::string& name, const std::string& suffix) const {
        return std::string(TMP_DIR) + name + suffix;
    }

    bool ReadFile(const std::string& path, std::string& content) const {
        std::ifstream in(path);
        if (!in.is_open()) return false;
        std::string line;
        while (std::getline(in, line)) {
            content += line;
        }
        return true;
    }

    std::string RemoveAllWhitespace(const std::string& str) const {
        std::string result;
        for (char c : str) {
            if (c != ' ' && c != '\n' && c != '\r' && c != '\t') {
                result += c;
            }
        }
        return result;
    }

    std::string HandlerEnd(const std::vector<Status>& result,
                           const std::string& file_name) {
        Json::Value out;
        std::string status_str;
        bool has_error = false;
        bool has_wa = false;

        for (auto& s : result) {
            status_str += StatusToString(s) + " ";
            if (s == WA) has_wa = true;
            if (s != AC && s != WA) has_error = true;
        }

        out["status"] = status_str;

        if (has_error) {
            out["desc"] = StatusToDesc(result.empty() ? UNKNOWN : result[0]);
        } else if (has_wa) {
            out["desc"] = StatusToDesc(WA);
        } else {
            out["desc"] = StatusToDesc(AC);
        }

        Json::StyledWriter writer;
        return writer.write(out);
    }
};

} // namespace oj_judge