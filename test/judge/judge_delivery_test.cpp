#include "judge_delivery_policy.hpp"

#include <cstdlib>
#include <iostream>

namespace
{
void Check(bool condition, const char* message)
{
    if (condition) return;
    std::cerr << "FAIL: " << message << '\n';
    std::exit(1);
}

oj::mq::JudgeTaskMessage ValidTask()
{
    oj::mq::JudgeTaskMessage task;
    task.set_message_id("message-id");
    task.set_submission_id(1);
    task.set_user_id(2);
    task.set_question_id("1000");
    task.set_code("int main(){}");
    task.set_language("cpp17");
    task.set_time_limit_ms(1000);
    task.set_memory_limit_mb(256);
    task.set_created_at(1700000000);
    task.add_test_cases()->set_index(1);
    return task;
}
}

int main()
{
    auto task = ValidTask();
    Check(oj::judge::ValidJudgeTask(task), "valid immutable task is accepted");
    task.clear_message_id();
    Check(!oj::judge::ValidJudgeTask(task), "missing message id is rejected");
    task = ValidTask();
    task.set_code(std::string(64 * 1024 + 1, 'x'));
    Check(!oj::judge::ValidJudgeTask(task), "oversized code is rejected");
    Check(oj::judge::ResolveDeliveryAction(oj::judge::ReportDecision::Ack, 0) ==
              oj::judge::DeliveryAction::Ack,
          "persisted callback ACKs delivery");
    Check(oj::judge::ResolveDeliveryAction(oj::judge::ReportDecision::Retry, 2) ==
              oj::judge::DeliveryAction::Retry,
          "retryable callback routes to delay queue");
    Check(oj::judge::ResolveDeliveryAction(oj::judge::ReportDecision::Retry, 3) ==
              oj::judge::DeliveryAction::DeadLetter,
          "third retry routes to DLQ");
    Check(oj::judge::ResolveDeliveryAction(oj::judge::ReportDecision::Reject, 0) ==
              oj::judge::DeliveryAction::DeadLetter,
          "rejected callback routes to DLQ");
    std::cout << "judge_delivery_test: PASS\n";
}
