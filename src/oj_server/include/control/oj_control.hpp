#pragma once

#include "control_auth.hpp"
#include "control_comment.hpp"
#include "control_solution.hpp"
#include "control_user.hpp"
#include "control_question.hpp"

// OnlineJudge Controller — 路由注册在 oj_server.cpp 中完成

namespace ns_control
{

    // 主Controller：组合各个业务模块
    class Control : public ControlBase
    {
    public:
        ControlAuth     _auth;
        ControlComment  _comment;
        ControlSolution _solution;
        ControlUser     _user;
        ControlQuestion _question;

        ControlAuth&     Auth()     { return _auth; }
        ControlComment&  Comment()  { return _comment; }
        ControlSolution& Solution() { return _solution; }
        ControlUser&     User()     { return _user; }
        ControlQuestion& Question() { return _question; }
    };

}
