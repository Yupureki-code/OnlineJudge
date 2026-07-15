#include "contract_test_support.hpp"

using namespace oj::legacy_contract;
using namespace oj::legacy_contract::test;

int main() {
    Checks checks;
    CheckSharedWireContract(checks);
    CheckRpcTable(checks, kUserRpcs);
    CheckPaging(checks, kUserPaging);

    for (const auto domain : {"user", "question", "solution", "comment", "submission", "static"}) {
        checks.Expect(HasDomain(kUserRpcs, domain), "every user-facing legacy domain has an RPC mapping");
    }
    checks.Expect(HasMethod(kUserRpcs, "UserService", "LoginWithPassword"), "password login is represented");
    checks.Expect(HasMethod(kUserRpcs, "UserService", "Register"), "code registration is represented");
    checks.Expect(HasMethod(kUserRpcs, "SubmissionService", "CreateSubmission"), "formal submission is asynchronous");
    checks.Expect(HasMethod(kUserRpcs, "SubmissionService", "CreateCustomTest"), "custom test is asynchronous");
    checks.Expect(HasMethod(kUserRpcs, "SubmissionService", "GetSubmission"), "submission polling is represented");

    int authenticated = 0;
    for (const auto& rpc : kUserRpcs) {
        if (rpc.auth == Auth::kUser) ++authenticated;
    }
    checks.Expect(authenticated >= 15, "state-changing user RPCs are session authenticated");
    checks.Expect(kUserPages.size() >= 10, "page and static resource URLs are inventoried");
    checks.Expect(kUserPages.back().status == 404, "missing and traversal paths return 404");
    checks.Expect(kRemovedLegacyUserRoutes.size() == 4,
                  "unsafe pre-authentication routes are explicitly removed");
    for (const auto& removed : kRemovedLegacyUserRoutes) {
        checks.Expect(!removed.reason.empty(), "removed legacy routes document a security reason");
    }

    return checks.Finish("oj_server_legacy_contract_test");
}
