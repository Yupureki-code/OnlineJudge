#pragma once

#include "control_base.hpp"

namespace ns_control
{

    class ControlAuth : public ControlBase
    {
    public:
        //发送邮箱验证码
        bool SendEmailAuthCode(const std::string& email, const std::string& client_ip, std::string* err_code, int* retry_after_seconds)
        {
            constexpr int kCodeTtlSeconds = 300;
            constexpr int kCooldownSeconds = 60;
            constexpr int kEmailDailyLimit = 20;
            constexpr int kIpDailyLimit = 50;

            if (retry_after_seconds != nullptr)
            {
                *retry_after_seconds = 0;
            }

            try
            {
                std::string cooldown_key = BuildAuthCooldownKey(email);
                if (_auth_redis.exists(cooldown_key))
                {
                    if (err_code != nullptr)
                    {
                        *err_code = "TOO_MANY_REQUESTS";
                    }
                    if (retry_after_seconds != nullptr)
                    {
                        *retry_after_seconds = kCooldownSeconds;
                    }
                    return false;
                }

                std::string email_daily_key = BuildAuthEmailDailyLimitKey(email);
                long long email_daily_count = _auth_redis.incr(email_daily_key);
                if (email_daily_count == 1)
                {
                    _auth_redis.expire(email_daily_key, SecondsUntilDayEnd());
                }
                if (email_daily_count > kEmailDailyLimit)
                {
                    if (err_code != nullptr)
                    {
                        *err_code = "EMAIL_DAILY_LIMIT";
                    }
                    return false;
                }

                std::string ip_daily_key = BuildAuthIpDailyLimitKey(client_ip);
                long long ip_daily_count = _auth_redis.incr(ip_daily_key);
                if (ip_daily_count == 1)
                {
                    _auth_redis.expire(ip_daily_key, SecondsUntilDayEnd());
                }
                if (ip_daily_count > kIpDailyLimit)
                {
                    if (err_code != nullptr)
                    {
                        *err_code = "IP_DAILY_LIMIT";
                    }
                    return false;
                }

                _auth_redis.setex(cooldown_key, kCooldownSeconds, "1");

                std::string code = GenerateAuthCode();
                _auth_redis.setex(BuildAuthCodeKey(email), kCodeTtlSeconds, code);
                _auth_redis.del(BuildAuthAttemptsKey(email));

                ns_mail::MailSendResult send_result = _mail.SendAuthCodeMail(email, code);
                if (!send_result.ok)
                {
                    _auth_redis.del(BuildAuthCodeKey(email));
                    _auth_redis.del(BuildAuthAttemptsKey(email));
                    _auth_redis.del(cooldown_key);
                    if (err_code != nullptr)
                    {
                        *err_code = "MAIL_SEND_FAILED";
                    }
                    return false;
                }

                if (retry_after_seconds != nullptr)
                {
                    *retry_after_seconds = kCooldownSeconds;
                }
                return true;
            }
            catch (const sw::redis::Error& e)
            {
                logger(ERROR) << "Redis auth send code failed: " << e.what();
                if (err_code != nullptr)
                {
                    *err_code = "REDIS_ERROR";
                }
                return false;
            }
        }
        //验证验证码，自动登录或者创建新用户
        bool VerifyEmailAuthCodeAndLogin(const std::string& email,
                         const std::string& code,
                         const std::string& optional_name,
                         const std::string& optional_password,
                         User* user,
                         bool* is_new_user,
                         std::string* err_code)
        {
            constexpr int kCodeTtlSeconds = 300;
            constexpr int kMaxAttempts = 5;

            if (user == nullptr)
            {
                if (err_code != nullptr)
                {
                    *err_code = "INVALID_ARGUMENT";
                }
                return false;
            }

            if (is_new_user != nullptr)
            {
                *is_new_user = false;
            }

            try
            {
                //从REDIS中获取验证码
                auto cached_code = _auth_redis.get(BuildAuthCodeKey(email));
                if (!cached_code)
                {
                    if (err_code != nullptr)
                    {
                        *err_code = "CODE_EXPIRED";
                    }
                    return false;
                }
                //判断验证码是否正确
                if (cached_code.value() != code)
                {
                    //验证码错误->尝试次数+1,大于等于5次删除验证码
                    std::string attempts_key = BuildAuthAttemptsKey(email);
                    long long attempts = _auth_redis.incr(attempts_key);
                    if (attempts == 1)
                    {
                        _auth_redis.expire(attempts_key, kCodeTtlSeconds);
                    }

                    if (attempts >= kMaxAttempts)
                    {
                        _auth_redis.del(BuildAuthCodeKey(email));
                        _auth_redis.del(attempts_key);
                        if (err_code != nullptr)
                        {
                            *err_code = "ATTEMPTS_EXCEEDED";
                        }
                    }
                    else
                    {
                        if (err_code != nullptr)
                        {
                            *err_code = "CODE_MISMATCH";
                        }
                    }
                    return false;
                }
                //验证码正确
                _auth_redis.del(BuildAuthCodeKey(email));
                _auth_redis.del(BuildAuthAttemptsKey(email));
                //查看该用户是否存在
                bool exists = _model.User().CheckUser(email);
                if (!exists)
                {
                    //用户不存在->创建账户
                    std::string trimmed_name = TrimSpace(optional_name);
                    if (trimmed_name.empty())
                    {
                        if (err_code != nullptr)
                        {
                            *err_code = "REGISTER_NAME_REQUIRED";
                        }
                        return false;
                    }

                    if (!IsPasswordStrongEnough(optional_password))
                    {
                        if (err_code != nullptr)
                        {
                            *err_code = "WEAK_PASSWORD";
                        }
                        return false;
                    }

                    std::string name = BuildDefaultUserName(email, trimmed_name);
                    if (!_model.User().CreateUser(name, email))
                    {
                        if (err_code != nullptr)
                        {
                            *err_code = "CREATE_USER_FAILED";
                        }
                        return false;
                    }

                    std::string password_hash = BuildPasswordHash(optional_password);
                    if (password_hash.empty())
                    {
                        if (err_code != nullptr)
                        {
                            *err_code = "PASSWORD_HASH_FAILED";
                        }
                        return false;
                    }

                    if (!_model.User().SetUserPassword(email, password_hash, PasswordAlgoTag()))
                    {
                        if (err_code != nullptr)
                        {
                            *err_code = "UPDATE_PASSWORD_FAILED";
                        }
                        return false;
                    }

                    if (is_new_user != nullptr)
                    {
                        *is_new_user = true;
                    }
                }

                _model.User().UpdateLastLogin(email);
                if (!_model.User().GetUser(email, user))
                {
                    if (err_code != nullptr)
                    {
                        *err_code = "LOAD_USER_FAILED";
                    }
                    return false;
                }
                return true;
            }
            catch (const sw::redis::Error& e)
            {
                logger(ERROR) << "Redis auth verify failed: " << e.what();
                if (err_code != nullptr)
                {
                    *err_code = "REDIS_ERROR";
                }
                return false;
            }
        }

        //用户:检查用户是否存在
        bool CheckUser(const std::string& email, Json::Value& response)
        {
            bool exists = _model.User().CheckUser(email);
            response["exists"] = exists;
            if(exists)
            {
                logger(ns_log::INFO)<<"用户存在 email:"<<email;
                User user;
                if(_model.User().GetUser(email, &user))
                {
                    response["user"] = Json::Value();
                    response["user"]["uid"] = user.uid;
                    response["user"]["name"] = user.name;
                    response["user"]["email"] = user.email;
                    response["user"]["create_time"] = user.create_time;
                    response["user"]["last_login"] = user.last_login;
                }
            }
            return true;
        }

        //用户:创建新用户
        bool CreateUser(const std::string& name, const std::string& email, Json::Value& response)
        {
            bool created = _model.User().CreateUser(name, email);
            response["created"] = created;
            if(created)
            {
                _model.User().UpdateLastLogin(email);
                User user;
                if(_model.User().GetUser(email, &user))
                {
                    response["user"] = Json::Value();
                    response["user"]["uid"] = user.uid;
                    response["user"]["name"] = user.name;
                    response["user"]["email"] = user.email;
                    response["user"]["create_time"] = user.create_time;
                    response["user"]["last_login"] = user.last_login;
                }
            }
            return created;
        }

        //用户:获取用户信息
        bool GetUser(const std::string& email, Json::Value& response)
        {
            User user;
            bool found = _model.User().GetUser(email, &user);
            if(found)
            {
                _model.User().UpdateLastLogin(email);
                response["user"] = Json::Value();
                response["user"]["uid"] = user.uid;
                response["user"]["name"] = user.name;
                response["user"]["email"] = user.email;
                response["user"]["create_time"] = user.create_time;
                response["user"]["last_login"] = user.last_login;
            }
            response["found"] = found;
            return found;
        }

        //用户:获取用户信息(User对象)
        bool GetUser(const std::string& email, User* user)
        {
            return _model.User().GetUser(email, user);
        }

        bool SetPasswordForUser(const std::string& email, const std::string& password, std::string* err_code)
        {
            if (!IsPasswordStrongEnough(password))
            {
                if (err_code != nullptr)
                {
                    *err_code = "WEAK_PASSWORD";
                }
                return false;
            }
            //创建哈希密码
            std::string password_hash = BuildPasswordHash(password);
            if (password_hash.empty())
            {
                if (err_code != nullptr)
                {
                    *err_code = "PASSWORD_HASH_FAILED";
                }
                return false;
            }
            //设置用户密码到MySQL中
            if (!_model.User().SetUserPassword(email, password_hash, PasswordAlgoTag()))
            {
                if (err_code != nullptr)
                {
                    *err_code = "UPDATE_PASSWORD_FAILED";
                }
                return false;
            }
            return true;
        }

        bool VerifyEmailAuthCode(const std::string& email,
                                 const std::string& code,
                                 std::string* err_code)
        {
            constexpr int kCodeTtlSeconds = 300;
            constexpr int kMaxAttempts = 5;

            try
            {
                auto cached_code = _auth_redis.get(BuildAuthCodeKey(email));
                if (!cached_code)
                {
                    if (err_code != nullptr)
                    {
                        *err_code = "CODE_EXPIRED";
                    }
                    return false;
                }

                if (cached_code.value() != code)
                {
                    std::string attempts_key = BuildAuthAttemptsKey(email);
                    long long attempts = _auth_redis.incr(attempts_key);
                    if (attempts == 1)
                    {
                        _auth_redis.expire(attempts_key, kCodeTtlSeconds);
                    }

                    if (attempts >= kMaxAttempts)
                    {
                        _auth_redis.del(BuildAuthCodeKey(email));
                        _auth_redis.del(attempts_key);
                        if (err_code != nullptr)
                        {
                            *err_code = "ATTEMPTS_EXCEEDED";
                        }
                    }
                    else
                    {
                        if (err_code != nullptr)
                        {
                            *err_code = "CODE_MISMATCH";
                        }
                    }
                    return false;
                }

                _auth_redis.del(BuildAuthCodeKey(email));
                _auth_redis.del(BuildAuthAttemptsKey(email));
                return true;
            }
            catch (const sw::redis::Error& e)
            {
                logger(ERROR) << "Redis auth verify failed: " << e.what();
                if (err_code != nullptr)
                {
                    *err_code = "REDIS_ERROR";
                }
                return false;
            }
        }
        //检查邮箱验证码正确性，若正确则更改邮箱
        bool ChangeEmailWithCode(const User& current_user,
                                 const std::string& new_email,
                                 const std::string& code,
                                 User* updated_user,
                                 std::string* err_code)
        {
            if (new_email == current_user.email)
            {
                if (err_code != nullptr)
                {
                    *err_code = "SAME_EMAIL";
                }
                return false;
            }
            //检查验证码正确性
            std::string verify_err;
            if (!VerifyEmailAuthCode(current_user.email, code, &verify_err))
            {
                if (err_code != nullptr)
                {
                    *err_code = verify_err;
                }
                return false;
            }
            //检查用户是否存在
            if (_model.User().CheckUser(new_email))
            {
                if (err_code != nullptr)
                {
                    *err_code = "EMAIL_ALREADY_EXISTS";
                }
                return false;
            }
            //更新邮箱
            if (!_model.User().UpdateUserEmail(current_user.email, new_email))
            {
                if (err_code != nullptr)
                {
                    *err_code = "UPDATE_EMAIL_FAILED";
                }
                return false;
            }

            if (updated_user != nullptr)
            {
                if (!_model.User().GetUser(new_email, updated_user))
                {
                    if (err_code != nullptr)
                    {
                        *err_code = "LOAD_USER_FAILED";
                    }
                    return false;
                }
            }
            return true;
        }
        //检查验证码正确性，若正确则更改密码
        bool ChangePasswordWithCode(const std::string& email,
                                    const std::string& code,
                                    const std::string& new_password,
                                    std::string* err_code)
        {
            std::string verify_err;
            if (!VerifyEmailAuthCode(email, code, &verify_err))
            {
                if (err_code != nullptr)
                {
                    *err_code = verify_err;
                }
                return false;
            }

            return SetPasswordForUser(email, new_password, err_code);
        }
        //检查验证码正确性，若正确则注销账户
        bool DeleteAccountWithCode(const std::string& email,
                                   const std::string& code,
                                   std::string* err_code)
        {
            std::string verify_err;
            if (!VerifyEmailAuthCode(email, code, &verify_err))
            {
                if (err_code != nullptr)
                {
                    *err_code = verify_err;
                }
                return false;
            }

            if (!_model.User().DeleteUserByEmail(email))
            {
                if (err_code != nullptr)
                {
                    *err_code = "DELETE_USER_FAILED";
                }
                return false;
            }
            return true;
        }
        //邮箱/用户名+密码登陆
        bool LoginWithPassword(const std::string& email_or_username,
                               const std::string& password,
                               User* user,
                               std::string* err_code)
        {
            if (user == nullptr)
            {
                if (err_code != nullptr)
                {
                    *err_code = "INVALID_ARGUMENT";
                }
                return false;
            }

            std::string password_hash;
            std::string password_algo;
            std::string resolved_email;
            bool found = false;

            // 先作为用户名查找
            User user_by_name;
            if (_model.User().GetUserByName(email_or_username, &user_by_name))
            {
                if (_model.User().GetUserPasswordAuth(user_by_name.email, &password_hash, &password_algo))
                {
                    resolved_email = user_by_name.email;
                    found = true;
                }
            }

            // 如果用户名方式没找到或密码为空，尝试作为邮箱查找
            if (!found)
            {
                password_hash.clear();
                password_algo.clear();
                if (_model.User().GetUserPasswordAuth(email_or_username, &password_hash, &password_algo))
                {
                    resolved_email = email_or_username;
                    found = true;
                }
            }

            if (!found || password_hash.empty())
            {
                if (err_code != nullptr)
                {
                    *err_code = "USER_NOT_FOUND";
                }
                return false;
            }

            if (password_algo.empty() || password_algo == "none")
            {
                if (err_code != nullptr)
                {
                    *err_code = "PASSWORD_NOT_SET";
                }
                return false;
            }
            //判断密码是否正确
            if (!VerifyPasswordHash(password, password_hash, password_algo))
            {
                if (err_code != nullptr)
                {
                    *err_code = "PASSWORD_MISMATCH";
                }
                return false;
            }
            //更新登陆时间
            _model.User().UpdateLastLogin(resolved_email);
            if (!_model.User().GetUser(resolved_email, user))
            {
                if (err_code != nullptr)
                {
                    *err_code = "LOAD_USER_FAILED";
                }
                return false;
            }
            return true;
        }

        bool LoginAdminWithIdAndPassword(int admin_id,
                                         const std::string& password,
                                         AdminAccount* admin,
                                         User* user,
                                         std::string* err_code)
        {
            if (admin == nullptr || user == nullptr)
            {
                if (err_code != nullptr)
                {
                    *err_code = "INVALID_ARGUMENT";
                }
                return false;
            }

            if (admin_id <= 0 || password.empty())
            {
                if (err_code != nullptr)
                {
                    *err_code = "INVALID_ARGUMENT";
                }
                return false;
            }

            if (!_model.GetAdminByAdminId(admin_id, admin))
            {
                if (err_code != nullptr)
                {
                    *err_code = "INVALID_CREDENTIALS";
                }
                return false;
            }

            if (!VerifyPasswordHash(password, admin->password_hash, PasswordAlgoTag()))
            {
                if (err_code != nullptr)
                {
                    *err_code = "INVALID_CREDENTIALS";
                }
                return false;
            }

            if (!_model.User().GetUserById(admin->uid, user))
            {
                if (err_code != nullptr)
                {
                    *err_code = "LOAD_USER_FAILED";
                }
                return false;
            }

            _model.User().UpdateLastLogin(user->email);
            return true;
        }

        bool BuildSecurePasswordHash(const std::string& password,
                                     std::string* password_hash,
                                     std::string* err_code)
        {
            if (password_hash == nullptr)
            {
                if (err_code != nullptr)
                {
                    *err_code = "INVALID_ARGUMENT";
                }
                return false;
            }

            if (!IsPasswordStrongEnough(password))
            {
                if (err_code != nullptr)
                {
                    *err_code = "WEAK_PASSWORD";
                }
                return false;
            }

            *password_hash = BuildPasswordHash(password);
            if (password_hash->empty())
            {
                if (err_code != nullptr)
                {
                    *err_code = "PASSWORD_HASH_FAILED";
                }
                return false;
            }
            return true;
        }

    protected:
        std::string PasswordAlgoTag() const
        {
            return "sha256_iter_v1";
        }

        int PasswordIterations() const
        {
            return 120000;
        }

        std::string PasswordPepper() const
        {
            const char* pepper = std::getenv("OJ_PASSWORD_PEPPER");
            if (pepper == nullptr)
            {
                return "";
            }
            return std::string(pepper);
        }
        //判断密码强度
        bool IsPasswordStrongEnough(const std::string& password) const
        {
            if (password.size() < 8 || password.size() > 72)
            {
                return false;
            }

            bool has_letter = false;
            bool has_digit = false;
            for (unsigned char ch : password)
            {
                if (std::isalpha(ch)) has_letter = true;
                if (std::isdigit(ch)) has_digit = true;
            }
            return has_letter && has_digit;
        }

        std::string ToHex(const unsigned char* data, size_t len) const
        {
            std::ostringstream oss;
            oss << std::hex << std::setfill('0');
            for (size_t i = 0; i < len; ++i)
            {
                oss << std::setw(2) << static_cast<unsigned int>(data[i]);
            }
            return oss.str();
        }

        std::string Sha256Hex(const std::string& input) const
        {
            std::array<unsigned char, SHA256_DIGEST_LENGTH> digest{};
            SHA256(reinterpret_cast<const unsigned char*>(input.data()), input.size(), digest.data());
            return ToHex(digest.data(), digest.size());
        }

        std::string GenerateSaltHex(size_t bytes) const
        {
            static thread_local std::mt19937 rng(std::random_device{}());
            std::uniform_int_distribution<int> dist(0, 255);
            std::vector<unsigned char> raw(bytes);
            for (size_t i = 0; i < bytes; ++i)
            {
                raw[i] = static_cast<unsigned char>(dist(rng));
            }
            return ToHex(raw.data(), raw.size());
        }

        std::string DerivePasswordDigest(const std::string& password, const std::string& salt, int iterations) const
        {
            std::string pepper = PasswordPepper();
            std::string digest = Sha256Hex(salt + ":" + password + ":" + pepper);
            for (int i = 1; i < iterations; ++i)
            {
                digest = Sha256Hex(digest + ":" + salt + ":" + pepper);
            }
            return digest;
        }

        std::string BuildPasswordHash(const std::string& password) const
        {
            const int iter = PasswordIterations();
            std::string salt = GenerateSaltHex(16);
            std::string digest = DerivePasswordDigest(password, salt, iter);
            if (digest.empty())
            {
                return "";
            }

            return salt + "$" + std::to_string(iter) + "$" + digest;
        }

        bool VerifyPasswordHash(const std::string& password,
                                const std::string& stored_hash,
                                const std::string& algo) const
        {
            if (algo != PasswordAlgoTag())
            {
                return false;
            }

            std::vector<std::string> parts;
            StringUtil::SplitString(stored_hash, "$", &parts);
            if (parts.size() != 3)
            {
                return false;
            }

            const std::string& salt = parts[0];
            int iter = 0;
            try
            {
                iter = std::stoi(parts[1]);
            }
            catch (...)
            {
                return false;
            }

            if (iter <= 0)
            {
                return false;
            }

            std::string expected = DerivePasswordDigest(password, salt, iter);
            return expected == parts[2];
        }

        std::string BuildAuthCodeKey(const std::string& email) const
        {
            return "oj:auth:code:" + email;
        }

        std::string BuildAuthAttemptsKey(const std::string& email) const
        {
            return "oj:auth:attempts:" + email;
        }
        //构建验证码发送冷却的redis key，格式为oj:auth:cooldown:{email}
        std::string BuildAuthCooldownKey(const std::string& email) const
        {
            return "oj:auth:cooldown:" + email;
        }

        std::string BuildAuthEmailDailyLimitKey(const std::string& email) const
        {
            return "oj:auth:limit:email:" + email + ":" + CurrentDateTag();
        }

        std::string BuildAuthIpDailyLimitKey(const std::string& ip) const
        {
            return "oj:auth:limit:ip:" + ip + ":" + CurrentDateTag();
        }

        std::string CurrentDateTag() const
        {
            auto now = std::chrono::system_clock::now();
            std::time_t tt = std::chrono::system_clock::to_time_t(now);
            std::tm tm_now;
#if defined(_WIN32)
            localtime_s(&tm_now, &tt);
#else
            localtime_r(&tt, &tm_now);
#endif
            char buf[16] = {0};
            std::strftime(buf, sizeof(buf), "%Y%m%d", &tm_now);
            return std::string(buf);
        }

        int SecondsUntilDayEnd() const
        {
            auto now = std::chrono::system_clock::now();
            std::time_t tt = std::chrono::system_clock::to_time_t(now);
            std::tm tm_now;
#if defined(_WIN32)
            localtime_s(&tm_now, &tt);
#else
            localtime_r(&tt, &tm_now);
#endif
            int seconds = (23 - tm_now.tm_hour) * 3600 + (59 - tm_now.tm_min) * 60 + (60 - tm_now.tm_sec);
            return std::max(60, seconds);
        }

        std::string GenerateAuthCode() const
        {
            static thread_local std::mt19937 rng(std::random_device{}());
            std::uniform_int_distribution<int> dist(0, 999999);
            int value = dist(rng);
            std::ostringstream oss;
            oss.width(6);
            oss.fill('0');
            oss << value;
            return oss.str();
        }

        std::string BuildDefaultUserName(const std::string& email, const std::string& optional_name) const
        {
            std::string name = TrimSpace(optional_name);
            if (!name.empty())
            {
                if (name.size() > 64)
                {
                    name.resize(64);
                }
                return name;
            }

            auto pos = email.find('@');
            std::string prefix = pos == std::string::npos ? email : email.substr(0, pos);
            if (prefix.empty())
            {
                prefix = "user";
            }
            if (prefix.size() > 32)
            {
                prefix.resize(32);
            }

            auto now = std::chrono::system_clock::now().time_since_epoch();
            long long suffix = std::chrono::duration_cast<std::chrono::seconds>(now).count() % 100000;
            return prefix + "_" + std::to_string(suffix);
        }
    };

}
