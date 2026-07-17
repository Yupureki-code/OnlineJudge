#pragma once

#include "control_base.hpp"
#include "../../../comm/models/user.hxx"
#include "../../../comm/models/admin.hxx"
#include <array>
#include <cstdint>
#include <openssl/crypto.h>
#include <openssl/rand.h>


namespace oj::control
{
    using oj::db::User;
    namespace
    {
        inline std::string CurrentDateTag()
        {
            auto now = std::chrono::system_clock::now();
            std::time_t tt = std::chrono::system_clock::to_time_t(now);
            std::tm tm_now;
            localtime_r(&tt, &tm_now);
            char buf[16] = {0};
            std::strftime(buf, sizeof(buf), "%Y%m%d", &tm_now);
            return std::string(buf);
        }

        inline int SecondsUntilDayEnd()
        {
            auto now = std::chrono::system_clock::now();
            std::time_t tt = std::chrono::system_clock::to_time_t(now);
            std::tm tm_now;
            localtime_r(&tt, &tm_now);
            int seconds = (23 - tm_now.tm_hour) * 3600 + (59 - tm_now.tm_min) * 60 + (60 - tm_now.tm_sec);
            return std::max(60, seconds);
        }

        inline std::string GenerateAuthCode()
        {
            uint32_t random_value = 0;
            constexpr uint32_t kRange = 1000000;
            constexpr uint32_t kLimit = UINT32_MAX - (UINT32_MAX % kRange);
            do
            {
                if (RAND_bytes(reinterpret_cast<unsigned char*>(&random_value), sizeof(random_value)) != 1)
                {
                    return {};
                }
            } while (random_value >= kLimit);
            int value = static_cast<int>(random_value % kRange);
            std::ostringstream oss;
            oss.width(6);
            oss.fill('0');
            oss << value;
            return oss.str();
        }
        inline std::string BuildDefaultUserName(const std::string& email, const std::string& optional_name)
        {
            std::string name = oj::util::StringUtil::TrimSpace(optional_name);
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
        inline std::string BuildAuthCodeKey(const std::string& email)
        {
            return "oj:auth:code:" + email;
        }

        inline std::string BuildAuthAttemptsKey(const std::string& email)
        {
            return "oj:auth:attempts:" + email;
        }
        //构建验证码发送冷却的redis key，格式为oj:auth:cooldown:{email}
        inline std::string BuildAuthCooldownKey(const std::string& email)
        {
            return "oj:auth:cooldown:" + email;
        }

        inline std::string BuildAuthEmailDailyLimitKey(const std::string& email)
        {
            return "oj:auth:limit:email:" + email + ":" + CurrentDateTag();
        }

        inline std::string BuildAuthIpDailyLimitKey(const std::string& ip)
        {
            return "oj:auth:limit:ip:" + ip + ":" + CurrentDateTag();
        }
        inline bool IsPasswordStrongEnough(const std::string& password)
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

        inline std::string PasswordAlgoTag()
        {
            return "sha256_iter_v1";
        }

        inline int PasswordIterations()
        {
            return 120000;
        }

        inline std::string PasswordPepper()
        {
            const char* pepper = std::getenv("OJ_PASSWORD_PEPPER");
            if (pepper == nullptr)
            {
                return "";
            }
            return std::string(pepper);
        }
        //判断密码强度
        std::string ToHex(const unsigned char* data, size_t len)
        {
            std::ostringstream oss;
            oss << std::hex << std::setfill('0');
            for (size_t i = 0; i < len; ++i)
            {
                oss << std::setw(2) << static_cast<unsigned int>(data[i]);
            }
            return oss.str();
        }

        std::string Sha256Hex(const std::string& input)
        {
            std::array<unsigned char, SHA256_DIGEST_LENGTH> digest{};
            SHA256(reinterpret_cast<const unsigned char*>(input.data()), input.size(), digest.data());
            return ToHex(digest.data(), digest.size());
        }

        std::string GenerateSaltHex(size_t bytes)
        {
            std::vector<unsigned char> raw(bytes);
            if (bytes == 0 || RAND_bytes(raw.data(), raw.size()) != 1)
            {
                return {};
            }
            return ToHex(raw.data(), raw.size());
        }

        std::string DerivePasswordDigest(const std::string& password, const std::string& salt, int iterations)
        {
            std::string pepper = PasswordPepper();
            std::string digest = Sha256Hex(salt + ":" + password + ":" + pepper);
            for (int i = 1; i < iterations; ++i)
            {
                digest = Sha256Hex(digest + ":" + salt + ":" + pepper);
            }
            return digest;
        }

        std::string BuildPasswordHash(const std::string& password)
        {
            const int iter = PasswordIterations();
            std::string salt = GenerateSaltHex(16);
            if (salt.empty())
            {
                return {};
            }
            std::string digest = DerivePasswordDigest(password, salt, iter);
            if (digest.empty())
            {
                return "";
            }

            return salt + "$" + std::to_string(iter) + "$" + digest;
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
        bool VerifyPasswordHash(const std::string& password,
                                const std::string& stored_hash,
                                const std::string& algo)
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

            if (iter != PasswordIterations() || salt.size() != 32 || parts[2].size() != SHA256_DIGEST_LENGTH * 2)
            {
                return false;
            }

            std::string expected = DerivePasswordDigest(password, salt, iter);
            return expected.size() == parts[2].size() &&
                   CRYPTO_memcmp(expected.data(), parts[2].data(), expected.size()) == 0;
        }

        inline std::string DummyPasswordHash()
        {
            static const std::string hash = []() {
                const std::string salt(32, '0');
                return salt + "$" + std::to_string(PasswordIterations()) + "$" +
                       DerivePasswordDigest("invalid-password", salt, PasswordIterations());
            }();
            return hash;
        }

        inline std::string LoginRateKey(const std::string& scope,
                                        const std::string& dimension,
                                        const std::string& value,
                                        const std::string& suffix)
        {
            return "oj:auth:password:" + scope + ":" + dimension + ":" +
                   Sha256Hex(value) + ":" + suffix;
        }

        inline bool CheckPasswordLoginRate(sw::redis::Redis& redis,
                                           const std::string& scope,
                                           const std::string& account,
                                           const std::string& client_ip,
                                           std::string* err_code)
        {
            try
            {
                static constexpr const char* kCheckScript = R"lua(
                    if redis.call('EXISTS', KEYS[1]) == 1 or redis.call('EXISTS', KEYS[2]) == 1 then
                        return 0
                    end
                    return 1
                )lua";
                const long long allowed = redis.eval<long long>(
                    kCheckScript,
                    {LoginRateKey(scope, "account", account, "lock"),
                     LoginRateKey(scope, "ip", client_ip, "lock")}, {});
                if (allowed != 1)
                {
                    if (err_code != nullptr) *err_code = "TOO_MANY_REQUESTS";
                    return false;
                }
                return true;
            }
            catch (const sw::redis::Error& e)
            {
                LOG_ERROR("{}{}", "Redis password login rate check failed: ", e.what());
                if (err_code != nullptr) *err_code = "REDIS_ERROR";
                return false;
            }
        }

        inline void RecordPasswordLoginFailure(sw::redis::Redis& redis,
                                               const std::string& scope,
                                               const std::string& account,
                                               const std::string& client_ip)
        {
            static constexpr const char* kFailureScript = R"lua(
                local window = tonumber(ARGV[1])
                local function validate(attempts_key)
                    local value = redis.call('GET', attempts_key)
                    if value then
                        local parsed = tonumber(value)
                        if not parsed or parsed < 0 or parsed ~= math.floor(parsed) then
                            return redis.error_reply('invalid login attempt counter')
                        end
                    end
                    return false
                end
                local account_error = validate(KEYS[1])
                if account_error then return account_error end
                local ip_error = validate(KEYS[3])
                if ip_error then return ip_error end
                local function record(attempts_key, lock_key, threshold)
                    local attempts = redis.call('INCR', attempts_key)
                    if attempts == 1 then redis.call('EXPIRE', attempts_key, window) end
                    if attempts >= threshold then
                        local exponent = math.min(6, attempts - threshold)
                        local lock_seconds = math.min(window, 5 * (2 ^ exponent))
                        redis.call('SETEX', lock_key, lock_seconds, '1')
                    end
                    return attempts
                end
                local account_attempts = record(KEYS[1], KEYS[2], tonumber(ARGV[2]))
                local ip_attempts = record(KEYS[3], KEYS[4], tonumber(ARGV[3]))
                return math.max(account_attempts, ip_attempts)
            )lua";
            redis.eval<long long>(
                kFailureScript,
                {LoginRateKey(scope, "account", account, "attempts"),
                 LoginRateKey(scope, "account", account, "lock"),
                 LoginRateKey(scope, "ip", client_ip, "attempts"),
                 LoginRateKey(scope, "ip", client_ip, "lock")},
                {"900", "5", "50"});
        }

        inline void ClearPasswordLoginAccountRate(sw::redis::Redis& redis,
                                                  const std::string& scope,
                                                  const std::string& account)
        {
            redis.del(LoginRateKey(scope, "account", account, "attempts"));
            redis.del(LoginRateKey(scope, "account", account, "lock"));
        }

        inline std::string BuildEmailChangeCodeKey(int uid, const std::string& new_email)
        {
            return "oj:auth:change-email:" + std::to_string(uid) + ":" + Sha256Hex(new_email) + ":code";
        }

        inline std::string BuildEmailChangeAttemptsKey(int uid, const std::string& new_email)
        {
            return "oj:auth:change-email:" + std::to_string(uid) + ":" + Sha256Hex(new_email) + ":attempts";
        }
    }
    class ControlAuth : public ControlBase
    {
    public:
        bool BuildSecurePasswordHash(const std::string& password,
                                     std::string* password_hash,
                                     std::string* err_code)
        {
            return oj::control::BuildSecurePasswordHash(password, password_hash, err_code);
        }

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
                if (code.empty())
                {
                    _auth_redis.del(cooldown_key);
                    if (err_code != nullptr) *err_code = "CSPRNG_FAILED";
                    return false;
                }
                _auth_redis.setex(BuildAuthCodeKey(email), kCodeTtlSeconds, code);
                _auth_redis.del(BuildAuthAttemptsKey(email));

                oj::mail::MailSendResult send_result = _mail.SendAuthCodeMail(email, code);
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
                LOG_ERROR("{}{}", "Redis auth send code failed: ", e.what());
                if (err_code != nullptr)
                {
                    *err_code = "REDIS_ERROR";
                }
                return false;
            }
        }

        bool SendEmailChangeCode(const User& current_user,
                                 const std::string& new_email,
                                 const std::string& client_ip,
                                 std::string* err_code,
                                 int* retry_after_seconds)
        {
            constexpr int kCodeTtlSeconds = 300;
            constexpr int kCooldownSeconds = 60;
            constexpr int kEmailDailyLimit = 20;
            constexpr int kIpDailyLimit = 50;
            if (retry_after_seconds != nullptr) *retry_after_seconds = 0;
            if (new_email == current_user.email)
            {
                if (err_code != nullptr) *err_code = "SAME_EMAIL";
                return false;
            }
            if (_model.User().CheckUser(new_email))
            {
                if (err_code != nullptr) *err_code = "EMAIL_ALREADY_EXISTS";
                return false;
            }

            try
            {
                const std::string cooldown_key = "oj:auth:change-email:cooldown:" +
                    std::to_string(current_user.uid) + ":" + Sha256Hex(new_email);
                if (_auth_redis.exists(cooldown_key))
                {
                    if (err_code != nullptr) *err_code = "TOO_MANY_REQUESTS";
                    if (retry_after_seconds != nullptr) *retry_after_seconds = kCooldownSeconds;
                    return false;
                }
                const std::string email_limit_key = BuildAuthEmailDailyLimitKey(new_email);
                const long long email_count = _auth_redis.incr(email_limit_key);
                if (email_count == 1) _auth_redis.expire(email_limit_key, SecondsUntilDayEnd());
                if (email_count > kEmailDailyLimit)
                {
                    if (err_code != nullptr) *err_code = "EMAIL_DAILY_LIMIT";
                    return false;
                }
                const std::string ip_limit_key = BuildAuthIpDailyLimitKey(client_ip);
                const long long ip_count = _auth_redis.incr(ip_limit_key);
                if (ip_count == 1) _auth_redis.expire(ip_limit_key, SecondsUntilDayEnd());
                if (ip_count > kIpDailyLimit)
                {
                    if (err_code != nullptr) *err_code = "IP_DAILY_LIMIT";
                    return false;
                }

                const std::string code = GenerateAuthCode();
                if (code.empty())
                {
                    if (err_code != nullptr) *err_code = "CSPRNG_FAILED";
                    return false;
                }
                _auth_redis.setex(cooldown_key, kCooldownSeconds, "1");
                _auth_redis.setex(BuildEmailChangeCodeKey(current_user.uid, new_email), kCodeTtlSeconds, code);
                _auth_redis.del(BuildEmailChangeAttemptsKey(current_user.uid, new_email));
                const auto send_result = _mail.SendAuthCodeMail(new_email, code);
                if (!send_result.ok)
                {
                    _auth_redis.del(BuildEmailChangeCodeKey(current_user.uid, new_email));
                    _auth_redis.del(BuildEmailChangeAttemptsKey(current_user.uid, new_email));
                    _auth_redis.del(cooldown_key);
                    if (err_code != nullptr) *err_code = "MAIL_SEND_FAILED";
                    return false;
                }
                if (retry_after_seconds != nullptr) *retry_after_seconds = kCooldownSeconds;
                return true;
            }
            catch (const sw::redis::Error& e)
            {
                LOG_ERROR("{}{}", "Redis email change code failed: ", e.what());
                if (err_code != nullptr) *err_code = "REDIS_ERROR";
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
                    std::string trimmed_name = oj::util::StringUtil::TrimSpace(optional_name);
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
                LOG_ERROR("{}{}", "Redis auth verify failed: ", e.what());
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
                LOG_INFO("User lookup matched an existing account");
                User user;
                if(_model.User().GetUser(email, &user))
                {
                    response["user"] = Json::Value();
                    response["user"]["uid"] = user.uid;
                    response["user"]["name"] = user.name;
                    response["user"]["email"] = user.email;
                    response["user"]["create_time"] = oj::util::TimeUtil::DateTimeToInt(user.create_time);
                    response["user"]["last_login"] = oj::util::TimeUtil::DateTimeToInt(user.last_login);
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
                    response["user"]["create_time"] = oj::util::TimeUtil::DateTimeToInt(user.create_time);
                    response["user"]["last_login"] = oj::util::TimeUtil::DateTimeToInt(user.last_login);
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
                response["user"]["create_time"] = oj::util::TimeUtil::DateTimeToInt(user.create_time);
                    response["user"]["last_login"] = oj::util::TimeUtil::DateTimeToInt(user.last_login);
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
                LOG_ERROR("{}{}", "Redis auth verify failed: ", e.what());
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
            constexpr int kCodeTtlSeconds = 300;
            constexpr int kMaxAttempts = 5;
            try
            {
                const std::string code_key = BuildEmailChangeCodeKey(current_user.uid, new_email);
                const std::string attempts_key = BuildEmailChangeAttemptsKey(current_user.uid, new_email);
                const auto cached_code = _auth_redis.get(code_key);
                if (!cached_code)
                {
                    if (err_code != nullptr) *err_code = "CODE_EXPIRED";
                    return false;
                }
                if (cached_code.value() != code)
                {
                    const long long attempts = _auth_redis.incr(attempts_key);
                    if (attempts == 1) _auth_redis.expire(attempts_key, kCodeTtlSeconds);
                    if (attempts >= kMaxAttempts)
                    {
                        _auth_redis.del(code_key);
                        _auth_redis.del(attempts_key);
                        if (err_code != nullptr) *err_code = "ATTEMPTS_EXCEEDED";
                    }
                    else if (err_code != nullptr)
                    {
                        *err_code = "CODE_MISMATCH";
                    }
                    return false;
                }
                _auth_redis.del(code_key);
                _auth_redis.del(attempts_key);
            }
            catch (const sw::redis::Error& e)
            {
                LOG_ERROR("{}{}", "Redis email change verify failed: ", e.what());
                if (err_code != nullptr) *err_code = "REDIS_ERROR";
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
                               const std::string& client_ip,
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

            if (!CheckPasswordLoginRate(_auth_redis, "user", email_or_username, client_ip, err_code))
            {
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

            const bool has_password = found && !password_hash.empty() &&
                                      password_algo == PasswordAlgoTag();
            const bool candidate_size_valid = password.size() <= 72;
            const bool password_matches = VerifyPasswordHash(
                candidate_size_valid ? password : std::string("invalid-password"),
                has_password ? password_hash : DummyPasswordHash(),
                PasswordAlgoTag());
            if (!has_password || !candidate_size_valid || !password_matches)
            {
                try
                {
                    RecordPasswordLoginFailure(_auth_redis, "user", email_or_username, client_ip);
                }
                catch (const sw::redis::Error& e)
                {
                    LOG_ERROR("{}{}", "Redis password login failure record failed: ", e.what());
                    if (err_code != nullptr) *err_code = "REDIS_ERROR";
                    return false;
                }
                if (err_code != nullptr) *err_code = "INVALID_CREDENTIALS";
                return false;
            }
            try
            {
                ClearPasswordLoginAccountRate(_auth_redis, "user", email_or_username);
            }
            catch (const sw::redis::Error& e)
            {
                LOG_ERROR("{}{}", "Redis password login success record failed: ", e.what());
                if (err_code != nullptr) *err_code = "REDIS_ERROR";
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
                                          const std::string& client_ip,
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

            const std::string account = std::to_string(admin_id);
            if (!CheckPasswordLoginRate(_auth_redis, "admin", account, client_ip, err_code))
            {
                return false;
            }

            const bool found = _model.GetAdminByAdminId(admin_id, admin);
            const bool candidate_size_valid = password.size() <= 72;
            const bool password_matches = VerifyPasswordHash(
                candidate_size_valid ? password : std::string("invalid-password"),
                found ? admin->password_hash : DummyPasswordHash(), PasswordAlgoTag());
            if (!found || !candidate_size_valid || !password_matches)
            {
                try
                {
                    RecordPasswordLoginFailure(_auth_redis, "admin", account, client_ip);
                }
                catch (const sw::redis::Error& e)
                {
                    LOG_ERROR("{}{}", "Redis admin login failure record failed: ", e.what());
                    if (err_code != nullptr) *err_code = "REDIS_ERROR";
                    return false;
                }
                if (err_code != nullptr) *err_code = "INVALID_CREDENTIALS";
                return false;
            }

            try
            {
                ClearPasswordLoginAccountRate(_auth_redis, "admin", account);
            }
            catch (const sw::redis::Error& e)
            {
                LOG_ERROR("{}{}", "Redis admin login success record failed: ", e.what());
                if (err_code != nullptr) *err_code = "REDIS_ERROR";
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
    };

}
