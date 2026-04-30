#pragma once

#include "control_question.hpp"

// OnlineJudge Controller — 路由注册在 oj_server.cpp 中完成

namespace ns_control
{

    // 主Controller：继承整个业务链
    class Control : public ControlQuestion
    {
    };

}
