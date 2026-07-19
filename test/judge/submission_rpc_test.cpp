#include "comm/proto/biz_service.pb.h"
#include "comm/proto/mq_message.pb.h"

#include <cstdlib>
#include <iostream>
#include <string>

namespace
{
void Check(bool condition, const char* message)
{
    if (condition) return;
    std::cerr << "FAIL: " << message << '\n';
    std::exit(1);
}
}

int main()
{
    oj::mq::JudgeTaskMessage formal;
    formal.set_message_id("0123456789abcdef0123456789abcdef");
    formal.set_submission_id(42);
    formal.set_user_id(7);
    formal.set_question_id("1000");
    formal.set_code("int main() { return 0; }");
    formal.set_language("cpp17");
    formal.set_time_limit_ms(1000);
    formal.set_memory_limit_mb(256);
    formal.set_created_at(1700000000);
    auto* test = formal.add_test_cases();
    test->set_test_case_id(9);
    test->set_index(1);
    test->set_input("1\n");
    test->set_expected_output("1\n");

    const std::string snapshot = formal.SerializeAsString();
    oj::mq::JudgeTaskMessage restored;
    Check(restored.ParseFromString(snapshot), "formal task snapshot parses");
    Check(restored.SerializeAsString() == snapshot, "formal task snapshot is immutable");
    Check(restored.has_submission_id() && !restored.has_custom_task_id(), "formal target is exclusive");
    Check(restored.test_cases(0).expected_output() == "1\n", "snapshot carries expected output");

    oj::mq::JudgeTaskMessage custom = formal;
    custom.clear_submission_id();
    custom.set_custom_task_id("custom-task-id-with-high-entropy");
    custom.set_custom_input("input\0bytes", 11);
    Check(custom.has_custom_task_id() && !custom.has_submission_id(), "custom target is exclusive");

    oj::biz::CreateSubmissionResponse response;
    response.mutable_status()->set_success(true);
    response.mutable_status()->set_code(200);
    response.mutable_submission()->set_submission_id(42);
    response.mutable_submission()->set_status(oj::common::SUBMISSION_STATUS_QUEUED);
    Check(response.submission().status() == oj::common::SUBMISSION_STATUS_QUEUED,
          "submission creation contract returns queued status");
    std::cout << "submission_rpc_test: PASS\n";
}
