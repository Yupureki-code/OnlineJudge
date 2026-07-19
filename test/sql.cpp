#include "comm.hpp"

// 用合法邮箱格式绕过前端校验，在密码字段注入
void TestLoginBypass() {
    TEST("SQL-登录绕过: test@a.com + SQL密码");
    std::string resp = HttpPostJson(BASE + "/api/user/login",
        R"({"email_or_username":"test@a.com","password":"' OR '1'='1"})");
    LOG("  POST 响应: " + resp);
    RESULT(resp.find("INVALID_CREDENTIALS") != std::string::npos ||
           resp.find("USER_NOT_FOUND") != std::string::npos);
}

void TestUnionSelect() {
    TEST("SQL-搜索框UNION");
    std::string resp = HttpGet(BASE + "/all_questions?q=1'+UNION+SELECT+1,password_hash,3,4,5,6,7,8+FROM+users--");
    LOG("  长度: " + std::to_string(resp.size()));
    RESULT(resp.find("password_hash") == std::string::npos);
}

void TestQuestionIdInject() {
    TEST("SQL-路径ID注入");
    std::string resp = HttpGet(BASE + "/api/question/1'+OR+'1'='1");
    LOG("  响应: " + resp.substr(0, 150));
    RESULT(resp.find("password_hash") == std::string::npos);
}

void TestSolutionInject() {
    TEST("SQL-题解列表注入");
    std::string resp = HttpGet(BASE + "/api/questions/1'+OR+'1'='1/solutions");
    LOG("  响应: " + resp.substr(0, 150));
    RESULT(resp.find("password_hash") == std::string::npos);
}

void TestCommentDrop() {
    TEST("SQL-评论DROP TABLE");
    std::string resp = HttpPostJson(BASE + "/api/solutions/11/comments",
        R"({"content":"test');DROP TABLE solution_comments;--"})", session_id);
    LOG("  POST: " + resp);
    RESULT(resp.find("success") != std::string::npos ||
           resp.find("UNAUTHORIZED") != std::string::npos);
}

int main() {
    curl_global_init(CURL_GLOBAL_ALL);
    TestLoginBypass();
    TestUnionSelect();
    TestQuestionIdInject();
    TestSolutionInject();
    TestCommentDrop();
    curl_global_cleanup();
}