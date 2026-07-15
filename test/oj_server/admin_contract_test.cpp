#include "contract_test_support.hpp"

using namespace oj::legacy_contract;
using namespace oj::legacy_contract::test;

int main() {
    Checks checks;
    CheckSharedWireContract(checks);
    CheckRpcTable(checks, kAdminRpcs);
    CheckPaging(checks, kAdminPaging);

    for (const auto domain : {"admin-auth", "admin", "admin-user", "admin-question",
                              "admin-test", "admin-account", "admin-audit"}) {
        checks.Expect(HasDomain(kAdminRpcs, domain), "every admin legacy domain has an RPC mapping");
    }
    checks.Expect(HasMethod(kAdminRpcs, "AdminService", "Login"), "admin login is represented");
    checks.Expect(HasMethod(kAdminRpcs, "AdminService", "GetOverview"), "admin overview is represented");
    checks.Expect(HasMethod(kAdminRpcs, "AdminService", "GetAuditLogs"), "audit paging is represented");

    int super_admin_methods = 0;
    for (const auto& rpc : kAdminRpcs) {
        if (rpc.auth == Auth::kSuperAdmin) ++super_admin_methods;
    }
    checks.Expect(super_admin_methods >= 7, "test-case, metrics, and account operations require super admin");
    checks.Expect(kAdminPages[3].behavior.find("unauthenticated is 404") != std::string_view::npos,
                  "admin shell preserves hidden unauthenticated behavior");
    checks.Expect(kAdminPages.front().path == "/healthz" && kAdminPages.front().status == 200,
                  "admin health endpoint is frozen");

    return checks.Finish("oj_admin_legacy_contract_test");
}
