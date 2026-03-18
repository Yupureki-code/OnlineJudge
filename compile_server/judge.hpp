#pragma once

#include <cstdio>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "COP_hanlder.hpp"
#include "../comm/logstrategy.hpp"

namespace ns_judge
{
    using namespace ns_hanlder;
    using namespace ns_util;
    using namespace ns_log;

    class HandlerJudge : public HandlerProgram
    {
    public:
        std::string Execute(const std::string& file_name,const std::string& in_json)override
        {
            return HandlerProgramEnd(AC, file_name);
        }
    };
};