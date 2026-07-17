from pathlib import Path
import shutil
import subprocess
import unittest


ROOT = Path(__file__).resolve().parents[2]


class UpstreamBoundaryTest(unittest.TestCase):
    def test_gateway_filters_service_cookies_and_overwrites_client_identity(self):
        source = (ROOT / "gateway/lua/brpc_upstream.lua").read_text()
        self.assertIn('"oj_admin_session_id" or "oj_session_id"', source)
        self.assertIn('["X-OJ-Client-IP"] = ngx.var.remote_addr', source)
        self.assertIn('["X-OJ-Gateway-Token"] = gateway_token()', source)
        self.assertNotIn('["Cookie"] = ngx.var.http_cookie', source)

        lua = shutil.which("lua")
        if lua is None:
            self.skipTest("lua is not installed")
        script = f'''
package.path = "{ROOT}/gateway/lua/?.lua;" .. package.path
package.preload["resty.http"] = function() return {{}} end
ngx = {{ var = {{}} , req = {{ get_headers = function() return {{}} end }} }}
local upstream = require "brpc_upstream"
local cookies = "oj_session_id=user-token; theme=dark; oj_admin_session_id=admin-token"
assert(upstream.service_cookie("oj.biz.OJService", cookies) == "oj_session_id=user-token")
assert(upstream.service_cookie("oj.biz.OJAdminService", cookies) == "oj_admin_session_id=admin-token")
assert(upstream.service_cookie("oj.biz.OJAdminService", "oj_session_id=user-token") == nil)
'''
        subprocess.run([lua, "-"], input=script, text=True, check=True)

    def test_compose_requires_production_credentials_and_enables_redis_acl(self):
        compose = (ROOT / "docker/docker-compose.yml").read_text()
        for variable in (
            "OJ_DB_PASSWORD", "OJ_REDIS_PASSWORD", "OJ_REDIS_ADDR", "OJ_MQ_PASSWORD",
            "OJ_JUDGE_CALLBACK_SECRET", "OJ_GATEWAY_AUTH_TOKEN_PATH",
        ):
            self.assertIn(f"${{{variable}:?", compose)
        self.assertIn("--user default on", compose)
        self.assertIn("gateway_auth_token", compose)


if __name__ == "__main__":
    unittest.main()
