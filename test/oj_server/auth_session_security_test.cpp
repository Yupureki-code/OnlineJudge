#include <cstdlib>
#include <iostream>
#include <string>

#include <sw/redis++/redis++.h>

#include "control/control_auth.hpp"
#include "oj_admin.hpp"
#include "oj_session.hpp"

namespace
{

void Check(bool condition, const std::string& message)
{
    if (!condition)
    {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(1);
    }
}

std::string RedisAddress()
{
    const char* address = std::getenv("OJ_TEST_REDIS_ADDR");
    return address == nullptr ? "tcp://127.0.0.1:6397" : address;
}

void CheckPasswordRateLimit(sw::redis::Redis& redis)
{
    const std::string account = "rate-user@example.test";
    const std::string ip = "192.0.2.20";
    std::string error;
    for (int i = 0; i < 3; ++i)
        Check(oj::control::CheckPasswordLoginRate(redis, "test", account, ip, &error),
              "rate precheck allows an unlocked login");

    const std::string account_attempts = oj::control::LoginRateKey("test", "account", account, "attempts");
    const std::string ip_attempts = oj::control::LoginRateKey("test", "ip", ip, "attempts");
    Check(!redis.exists(account_attempts) && !redis.exists(ip_attempts),
          "successful prechecks do not consume failure quota");

    for (int i = 0; i < 5; ++i)
        oj::control::RecordPasswordLoginFailure(redis, "test", account, ip);
    Check(redis.get(account_attempts).value_or("") == "5" && redis.get(ip_attempts).value_or("") == "5",
          "failure records account and IP together");
    error.clear();
    Check(!oj::control::CheckPasswordLoginRate(redis, "test", account, ip, &error) &&
              error == "TOO_MANY_REQUESTS",
          "failure threshold activates backoff");

    const std::string corrupt_account = "corrupt@example.test";
    const std::string corrupt_ip = "192.0.2.21";
    const std::string corrupt_account_key =
        oj::control::LoginRateKey("test", "account", corrupt_account, "attempts");
    const std::string corrupt_ip_key = oj::control::LoginRateKey("test", "ip", corrupt_ip, "attempts");
    redis.set(corrupt_ip_key, "not-an-integer");
    bool rejected = false;
    try
    {
        oj::control::RecordPasswordLoginFailure(redis, "test", corrupt_account, corrupt_ip);
    }
    catch (const sw::redis::Error&)
    {
        rejected = true;
    }
    Check(rejected && !redis.exists(corrupt_account_key),
          "corrupt counters fail closed before either dimension is changed");
}

void CheckUserSessions(sw::redis::Redis& redis, const std::string& address)
{
    oj::session::SessionManager sessions(address);
    const std::string missing_field = sessions.CreateSession(7, "user@example.test");
    Check(!missing_field.empty(), "user session created");
    redis.hdel("oj:prod:v1:session:user:" + missing_field, "email");
    oj::session::Session loaded;
    Check(!sessions.GetSession(missing_field, &loaded), "missing user hash field is rejected");

    const std::string extra_field = sessions.CreateSession(8, "other@example.test");
    redis.hset("oj:prod:v1:session:user:" + extra_field, "unexpected", "1");
    Check(!sessions.GetSession(extra_field, &loaded), "extra user hash field is rejected");

    oj::session::SessionManager unavailable("tcp://127.0.0.1:1");
    Check(!unavailable.DestroySession("unconfirmed"), "user revocation outage is retryable");
}

void CheckAdminSessions(sw::redis::Redis& redis, const std::string& address)
{
    oj::admin::AdminSessionStore sessions(true, address);
    oj::db::AdminAccount admin;
    admin.admin_id = 11;
    admin.uid = 7;
    admin.role = "admin";
    oj::db::User user;
    user.uid = 7;
    user.name = "Admin";
    user.email = "admin@example.test";

    const std::string session_id = sessions.CreateSession(admin, user);
    Check(!session_id.empty(), "admin session created");
    redis.hset("oj:prod:v1:session:admin:" + session_id, "uid", "7x");
    oj::admin::AdminSession loaded;
    Check(!sessions.GetSessionByCookie("oj_admin_session_id=" + session_id, &loaded),
          "malformed admin integer is rejected without throwing");

    oj::admin::AdminSessionStore unavailable(true, "tcp://127.0.0.1:1");
    Check(!unavailable.DestroySessionByCookie("oj_admin_session_id=unconfirmed"),
          "admin revocation outage is retryable");
}

} // namespace

int main()
{
    const std::string address = RedisAddress();
    sw::redis::Redis redis(address);
    redis.flushdb();
    CheckPasswordRateLimit(redis);
    CheckUserSessions(redis, address);
    CheckAdminSessions(redis, address);
    redis.flushdb();
    std::cout << "auth_session_security_test: PASS\n";
    return 0;
}
