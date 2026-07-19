// test/test_jwt.cpp
// JWT 签发/验证单元测试

#include "../src/comm/jwt.hpp"
#include <cassert>
#include <iostream>
#include <thread>
#include <chrono>

using namespace ns_jwt;

static void InitJWT() {
    JWTConfig config;
    config.secret = "test_secret_key_for_unit_test_only";
    config.ttl_seconds = 2;  // 2秒过期，便于测试
    config.issuer = "test_issuer";
    JWTManager::Instance().Init(config);
}

void test_sign_and_verify() {
    std::cout << "[TEST] Sign and Verify...";

    std::string token = JWTManager::Instance().Sign(12345, "test@example.com", 1);
    assert(!token.empty());

    auto claims = JWTManager::Instance().Verify(token);
    assert(claims.has_value());
    assert(claims->user_id == 12345);
    assert(claims->email == "test@example.com");
    assert(claims->role == 1);
    assert(!claims->jti.empty());

    std::cout << " PASSED" << std::endl;
}

void test_expired_token() {
    std::cout << "[TEST] Expired token...";

    std::string token = JWTManager::Instance().Sign(1, "a@b.com", 0);

    // 等待过期（TTL=2s）
    std::this_thread::sleep_for(std::chrono::seconds(3));

    auto claims = JWTManager::Instance().Verify(token);
    assert(!claims.has_value());  // 应验证失败

    std::cout << " PASSED" << std::endl;
}

void test_tampered_token() {
    std::cout << "[TEST] Tampered token...";

    std::string token = JWTManager::Instance().Sign(1, "a@b.com", 0);

    // 篡改 token（修改最后一个字符）
    std::string tampered = token;
    char last = tampered.back();
    tampered.back() = (last == 'A') ? 'B' : 'A';

    auto claims = JWTManager::Instance().Verify(tampered);
    assert(!claims.has_value());  // 应验证失败

    std::cout << " PASSED" << std::endl;
}

void test_invalid_token() {
    std::cout << "[TEST] Invalid token...";

    auto claims = JWTManager::Instance().Verify("invalid.token.here");
    assert(!claims.has_value());

    auto claims2 = JWTManager::Instance().Verify("");
    assert(!claims2.has_value());

    std::cout << " PASSED" << std::endl;
}

void test_wrong_secret() {
    std::cout << "[TEST] Wrong secret...";

    std::string token = JWTManager::Instance().Sign(1, "a@b.com", 0);

    // 用不同密钥初始化新管理器
    JWTConfig config;
    config.secret = "different_secret";
    config.ttl_seconds = 3600;
    config.issuer = "test_issuer";
    JWTManager::Instance().Init(config);

    auto claims = JWTManager::Instance().Verify(token);
    assert(!claims.has_value());  // 应验证失败

    std::cout << " PASSED" << std::endl;
}

int main() {
    std::cout << "=== JWT Tests ===" << std::endl;
    InitJWT();

    test_sign_and_verify();
    test_invalid_token();
    test_tampered_token();
    test_wrong_secret();
    test_expired_token();

    std::cout << "=== All tests passed ===" << std::endl;
    return 0;
}