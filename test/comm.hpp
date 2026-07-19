#include <curl/curl.h>
#include <string>
#include <iostream>
#include <cstring>

const std::string session_id = "ca443358fac23d566faf3abd5897354c";
const std::string BASE = "http://110.42.134.10:8080";

// libcurl 写回调
static size_t WriteCB(void* data, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)data, size * nmemb);
    return size * nmemb;
}

// 集合常用 Headers
struct curl_slist* SetCommonHeaders(const std::string& session = "") {
    struct curl_slist* h = nullptr;
    h = curl_slist_append(h, "Content-Type: application/json");
    if (!session.empty())
        h = curl_slist_append(h, ("Cookie: oj_session_id=" + session).c_str());
    return h;
}

// 执行 GET 并返回响应
std::string HttpGet(const std::string& url, const std::string& session = "",
                     long* http_code = nullptr) {
    CURL* c = curl_easy_init();
    std::string resp;
    curl_easy_setopt(c, CURLOPT_URL, url.c_str());
    curl_easy_setopt(c, CURLOPT_WRITEFUNCTION, WriteCB);
    curl_easy_setopt(c, CURLOPT_WRITEDATA, &resp);
    struct curl_slist* h = SetCommonHeaders(session);
    curl_easy_setopt(c, CURLOPT_HTTPHEADER, h);
    curl_easy_perform(c);
    if (http_code) curl_easy_getinfo(c, CURLINFO_RESPONSE_CODE, http_code);
    curl_slist_free_all(h);
    curl_easy_cleanup(c);
    return resp;
}

// 执行 POST JSON 并返回响应 + 状态码
std::string HttpPostJson(const std::string& url, const std::string& body,
                          const std::string& session = "", long* http_code = nullptr) {
    CURL* c = curl_easy_init();
    std::string resp;
    curl_easy_setopt(c, CURLOPT_URL, url.c_str());
    curl_easy_setopt(c, CURLOPT_POST, 1L);
    curl_easy_setopt(c, CURLOPT_POSTFIELDS, body.c_str());
    curl_easy_setopt(c, CURLOPT_POSTFIELDSIZE, body.size());
    curl_easy_setopt(c, CURLOPT_WRITEFUNCTION, WriteCB);
    curl_easy_setopt(c, CURLOPT_WRITEDATA, &resp);
    struct curl_slist* h = SetCommonHeaders(session);
    curl_easy_setopt(c, CURLOPT_HTTPHEADER, h);
    curl_easy_perform(c);
    if (http_code) curl_easy_getinfo(c, CURLINFO_RESPONSE_CODE, http_code);
    curl_slist_free_all(h);
    curl_easy_cleanup(c);
    return resp;
}

#define TEST(name) std::cout << "\n=== " << name << " ===" << std::endl
#define LOG(msg) std::cout << msg << std::endl
#define RESULT(cond) std::cout << "  结果: " << (cond ? "✅ 通过" : "❌ 失败") << std::endl