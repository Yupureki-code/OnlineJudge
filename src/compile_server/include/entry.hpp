#pragma once

#include <cstdio>
#include <memory>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "COP_hanlder.hpp"
#include "compiler.hpp"
#include "preprocesser.hpp"
#include "runner.hpp"
#include "judge.hpp"

//责任链的入口

namespace ns_entry
{
    using namespace ns_hanlder;

    class HandlerEntry
    {
    public:
        HandlerEntry()
        {
            _preprocesser = std::make_shared<ns_preprocesser::HandlerPreProcesser>();
            _complier = std::make_shared<ns_compile::HandlerCompiler>();
            _runner = std::make_shared<ns_runner::HandlerRunner>();
            _judge = std::make_shared<ns_judge::HandlerJudge>();

            _preprocesser->Set_next(_complier);
            _complier->Set_next(_runner);
            _runner->Set_next((_judge));
        }
        std::string Run(std::string& in_json)
        {
            return _preprocesser->Execute("",in_json);
        }
    private:
        std::shared_ptr<HandlerProgram> _preprocesser;
        std::shared_ptr<HandlerProgram> _complier;
        std::shared_ptr<HandlerProgram> _runner;
        std::shared_ptr<HandlerProgram> _judge;
    };
};
