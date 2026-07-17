#pragma once

#include "report_decision.hpp"
#include "../../comm/proto/mq_message.pb.h"

#include <cstddef>
#include <cstdint>

namespace oj::judge
{

inline bool ValidJudgeTask(const oj::mq::JudgeTaskMessage& task)
{
    if (task.message_id().empty() || task.user_id() == 0 || task.question_id().empty() ||
        task.code().empty() || task.code().size() > 64 * 1024 || task.language().empty() ||
        task.time_limit_ms() == 0 || task.time_limit_ms() > 60000 ||
        task.memory_limit_mb() == 0 || task.memory_limit_mb() > 4096 ||
        task.test_cases().empty() || task.test_cases_size() > 1000 || task.created_at() <= 0 ||
        (!task.has_submission_id() && !task.has_custom_task_id())) return false;
    for (const auto& test : task.test_cases())
        if (test.input().size() > 1024 * 1024 || test.expected_output().size() > 1024 * 1024)
            return false;
    return true;
}

} // namespace oj::judge
