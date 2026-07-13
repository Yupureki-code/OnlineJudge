#pragma once
#include "handler_base.hpp"
#include "preprocesser.hpp"
#include "compiler.hpp"
#include "runner.hpp"
#include "judge.hpp"
#include <memory>

namespace oj_judge {

/// 判题链入口：组装 preprocesser → compiler → runner → judge
class HandlerEntry 
{
public:
    HandlerEntry() 
    {
        _preprocesser = std::make_shared<HandlerPreProcesser>();
        _compiler = std::make_shared<HandlerCompiler>();
        _runner = std::make_shared<HandlerRunner>();
        _judge = std::make_shared<HandlerJudge>();

        _preprocesser->SetNext(_compiler);
        _compiler->SetNext(_runner);
        _runner->SetNext(_judge);
    }

    /// 运行判题链
    /// @param in_json 输入 JSON（包含 code, id, is_custom_test 等）
    /// @return 判题结果 JSON
    std::string Run(const oj_judge::SubmitRequest& request,std::coroutine_handle<> h) 
    {
        return _preprocesser->Execute("", request,h);
    }

    /// 获取 Runner 引用（用于注入测试用例或启用 Docker）
    std::shared_ptr<HandlerRunner> GetRunner() { return _runner; }

    /// 设置测试用例
    void SetTestCases(const std::vector<TestCase>& tests) 
    {
        _runner->SetTestCases(tests);
    }

private:
    std::shared_ptr<HandlerProgram> _preprocesser;
    std::shared_ptr<HandlerProgram> _compiler;
    std::shared_ptr<HandlerRunner> _runner;
    std::shared_ptr<HandlerProgram> _judge;
};

} // namespace oj_judge