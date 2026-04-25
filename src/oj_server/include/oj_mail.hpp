#pragma once

#include <algorithm>
#include <cctype>
#include <cstring>
#include <cstdlib>
#include <curl/curl.h>
#include <Logger/logstrategy.h>
#include <mutex>
#include <sstream>
#include <string>

#include "../../comm/comm.hpp"

namespace ns_mail
{
    using namespace ns_log;
    using namespace ns_util;
    struct MailSendResult
    {
        bool ok = false;
        std::string error;
    };

    namespace
    {
        std::string NormalizeMailAddress(std::string email)
        {
            email = StringUtil::Trim(email);
            if (!email.empty() && email.front() == '<' && email.back() == '>')
            {
                email = email.substr(1, email.size() - 2);
            }
            return email;
        }

        std::string BuildMailUrl()
        {
            std::ostringstream oss;
            if (smtp_port == 587)
            {
                oss << "smtp://";
            }
            else if (smtp_ssl)
            {
                oss << "smtps://";
            }
            else
            {
                oss << "smtp://";
            }
            oss << smtp_host << ":" << smtp_port;
            return oss.str();
        }

        std::string BuildFromAddress()
        {
            std::string user_email = NormalizeMailAddress(smtp_user);
            std::string from_email = NormalizeMailAddress(smtp_from);
            if (from_email.empty() || from_email == "null")
            {
                return user_email;
            }
            if (from_email != user_email)
            {
                logger(WARNING) << "smtp_from differs from smtp_user, fallback to smtp_user";
                return user_email;
            }
            return from_email;
        }

        struct UploadStatus
        {
            size_t bytes_read = 0;
            std::string payload;
        };

        struct DebugStatus
        {
            std::string server_lines;
        };

        size_t PayloadSource(char* ptr, size_t size, size_t nmemb, void* userp)
        {
            UploadStatus* upload = static_cast<UploadStatus*>(userp);
            size_t buffer_size = size * nmemb;
            if (upload == nullptr || buffer_size == 0)
            {
                return 0;
            }

            size_t left = upload->payload.size() - upload->bytes_read;
            size_t copy_size = left > buffer_size ? buffer_size : left;
            if (copy_size > 0)
            {
                std::memcpy(ptr, upload->payload.data() + upload->bytes_read, copy_size);
                upload->bytes_read += copy_size;
            }
            return copy_size;
        }

        int DebugCallback(CURL* handle, curl_infotype type, char* data, size_t size, void* userptr)
        {
            (void)handle;
            if (userptr == nullptr || data == nullptr || size == 0)
            {
                return 0;
            }

            if (type != CURLINFO_HEADER_IN)
            {
                return 0;
            }

            DebugStatus* dbg = static_cast<DebugStatus*>(userptr);
            std::string line(data, size);
            line.erase(std::remove(line.begin(), line.end(), '\r'), line.end());
            line.erase(std::remove(line.begin(), line.end(), '\n'), line.end());
            if (line.empty())
            {
                return 0;
            }

            if (dbg->server_lines.size() < 4096)
            {
                dbg->server_lines += line;
                dbg->server_lines += " | ";
            }
            return 0;
        }
    }

    class Mail
    {
    public:
        Mail() = default;
        ~Mail() = default;

        Mail(const Mail&) = default;
        Mail& operator=(const Mail&) = default;
        Mail(Mail&&) = default;
        Mail& operator=(Mail&&) = default;

        MailSendResult SendAuthCodeMail(const std::string& to, const std::string& code)
        {
            MailSendResult result;
            std::string username = StringUtil::Trim(smtp_user);
            std::string password = StringUtil::Trim(smtp_passwd);

            if (smtp_host.empty() || smtp_host == "null" ||
                username.empty() || username == "null" ||
                password.empty() || password == "null")
            {
                result.error = "smtp_not_configured";
                return result;
            }

            static std::once_flag curl_init_once;
            std::call_once(curl_init_once, []() {
                curl_global_init(CURL_GLOBAL_DEFAULT);
            });

            CURL* curl = curl_easy_init();
            if (curl == nullptr)
            {
                result.error = "curl_init_failed";
                return result;
            }

            std::string from = BuildFromAddress();
            std::ostringstream payload;
            payload << "To: <" << to << ">\r\n";
            payload << "From: <" << from << ">\r\n";
            payload << "Subject: Yupureki-OJ 邮箱验证码\r\n";
            payload << "Content-Type: text/plain; charset=UTF-8\r\n";
            payload << "\r\n";
            payload << "您正在登录 Yupureki-OJ。\r\n";
            payload << "验证码：" << code << "\r\n";
            payload << "有效期:5 分钟。\r\n";
            payload << "如果不是您本人操作，请忽略此邮件。\r\n";

            UploadStatus upload;
            upload.payload = payload.str();

            std::string url = BuildMailUrl();
            std::string from_mail = "<" + from + ">";
            std::string to_mail = "<" + to + ">";
            long use_ssl_mode = (smtp_port == 587 || smtp_ssl)
                ? static_cast<long>(CURLUSESSL_ALL)
                : static_cast<long>(CURLUSESSL_NONE);
            char error_buffer[CURL_ERROR_SIZE] = {0};
            DebugStatus dbg;
            bool smtp_debug = false;
            const char* debug_env = std::getenv("OJ_SMTP_DEBUG");
            if (debug_env != nullptr && std::string(debug_env) == "1")
            {
                smtp_debug = true;
            }

            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl, CURLOPT_USERNAME, username.c_str());
            curl_easy_setopt(curl, CURLOPT_PASSWORD, password.c_str());
            curl_easy_setopt(curl, CURLOPT_LOGIN_OPTIONS, "AUTH=LOGIN");
            curl_easy_setopt(curl, CURLOPT_USE_SSL, use_ssl_mode);
            curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, error_buffer);
            curl_easy_setopt(curl, CURLOPT_MAIL_FROM, from_mail.c_str());

            struct curl_slist* recipients = nullptr;
            recipients = curl_slist_append(recipients, to_mail.c_str());
            curl_easy_setopt(curl, CURLOPT_MAIL_RCPT, recipients);
            curl_easy_setopt(curl, CURLOPT_READFUNCTION, PayloadSource);
            curl_easy_setopt(curl, CURLOPT_READDATA, &upload);
            curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
            curl_easy_setopt(curl, CURLOPT_TIMEOUT, 20L);
            curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
            if (smtp_debug)
            {
                curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
                curl_easy_setopt(curl, CURLOPT_DEBUGFUNCTION, DebugCallback);
                curl_easy_setopt(curl, CURLOPT_DEBUGDATA, &dbg);
            }

            CURLcode res = curl_easy_perform(curl);
            if (res != CURLE_OK)
            {
                long smtp_response_code = 0;
                curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &smtp_response_code);
                result.error = error_buffer[0] != '\0' ? std::string(error_buffer) : std::string(curl_easy_strerror(res));
                logger(ERROR) << "Send mail failed by libcurl, code=" << static_cast<int>(res)
                              << " smtp_reply=" << smtp_response_code
                              << " detail=" << result.error;
                if (!dbg.server_lines.empty())
                {
                    logger(ERROR) << "SMTP transcript: " << dbg.server_lines;
                }
            }
            else
            {
                result.ok = true;
            }

            if (recipients != nullptr)
            {
                curl_slist_free_all(recipients);
            }
            curl_easy_cleanup(curl);

            return result;
        }
    };
}