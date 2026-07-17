import http.client
import os
import ssl
import unittest
from urllib.parse import urlparse


def request(url, method="GET", path="/healthz", body=None, headers=None):
    parsed = urlparse(url)
    context = ssl._create_unverified_context() if parsed.scheme == "https" else None
    connection_type = http.client.HTTPSConnection if parsed.scheme == "https" else http.client.HTTPConnection
    connection = connection_type(parsed.hostname, parsed.port, timeout=5, context=context) \
        if parsed.scheme == "https" else connection_type(parsed.hostname, parsed.port, timeout=5)
    connection.request(method, path, body=body, headers=headers or {})
    response = connection.getresponse()
    payload = response.read()
    result = response.status, response.getheaders(), payload
    connection.close()
    return result


class GatewaySecurityTest(unittest.TestCase):
    gateway = os.environ.get("OJ_GATEWAY_URL", "https://localhost")
    http_gateway = os.environ.get("OJ_GATEWAY_HTTP_URL", "http://localhost")

    @classmethod
    def setUpClass(cls):
        try:
            request(cls.gateway)
        except OSError as error:
            raise unittest.SkipTest(f"gateway is not running: {error}")

    def csrf(self, path="/api/csrf"):
        status, headers, payload = request(self.gateway, path=path)
        self.assertEqual(status, 200)
        cookie = next(value for name, value in headers if name.lower() == "set-cookie")
        token = cookie.split(";", 1)[0].split("=", 1)[1]
        return token, cookie, payload

    def test_csrf_cookie_attributes(self):
        _, cookie, _ = self.csrf()
        self.assertIn("SameSite=Lax", cookie)
        self.assertIn("Secure", cookie)
        self.assertNotIn("HttpOnly", cookie)

    def test_missing_and_wrong_csrf_are_rejected(self):
        token, _, _ = self.csrf()
        base = {"Content-Type": "application/json", "Origin": "https://localhost"}
        self.assertEqual(request(self.gateway, "POST", "/api/auth/logout", "{}", base)[0], 403)
        bad = dict(base, **{"Cookie": f"oj_csrf_token={token}", "X-CSRF-Token": "wrong"})
        self.assertEqual(request(self.gateway, "POST", "/api/auth/logout", "{}", bad)[0], 403)

    def test_bad_origin_is_rejected_even_for_login(self):
        token, _, _ = self.csrf()
        headers = {
            "Content-Type": "application/json", "Origin": "https://attacker.invalid",
            "Cookie": f"oj_csrf_token={token}", "X-CSRF-Token": token,
        }
        self.assertEqual(request(self.gateway, "POST", "/api/auth/login/password", "{}", headers)[0], 403)

    def test_http_redirect_and_health_exception(self):
        self.assertEqual(request(self.http_gateway, path="/healthz")[0], 200)
        self.assertEqual(request(self.http_gateway, path="/")[0], 308)

    def test_path_confusion_and_oversized_body(self):
        self.assertIn(request(self.gateway, path="/api//questions")[0], (400, 404))
        token, _, _ = self.csrf()
        headers = {
            "Content-Type": "application/json", "Origin": "https://localhost",
            "Cookie": f"oj_csrf_token={token}", "X-CSRF-Token": token,
        }
        body = '{"email":"' + ("a" * 1_100_000) + '"}'
        self.assertEqual(request(self.gateway, "POST", "/api/auth/registration-code", body, headers)[0], 413)


if __name__ == "__main__":
    unittest.main()
