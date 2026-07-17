import json
import os
import unittest

from _contracts import parse_routes
from security_test import request


class JsonGatewayE2ETest(unittest.TestCase):
    gateway = os.environ.get("OJ_GATEWAY_URL", "https://localhost")

    @classmethod
    def setUpClass(cls):
        try:
            request(cls.gateway)
        except OSError as error:
            raise unittest.SkipTest(f"gateway is not running: {error}")

    def test_static_files_are_served_directly(self):
        status, headers, body = request(self.gateway, path="/")
        self.assertEqual(status, 200)
        self.assertIn(b"<!DOCTYPE html>", body)
        self.assertTrue(any(name.lower() == "content-type" and "text/html" in value
                            for name, value in headers))
        self.assertEqual(request(self.gateway, "HEAD", "/css/common.css")[0], 200)
        self.assertEqual(request(self.gateway, path="/missing-phase8-file")[0], 404)

    def test_csrf_bootstrap_is_json(self):
        status, _, body = request(self.gateway, path="/api/csrf")
        self.assertEqual(status, 200)
        self.assertGreaterEqual(len(json.loads(body)["csrf_token"]), 40)

    def test_real_protobuf_round_trip_preserves_contract_shapes(self):
        status, _, body = request(self.gateway, path="/admin/api/version")
        response = json.loads(body)
        self.assertEqual(status, 200)
        self.assertTrue(response["status"]["success"])
        self.assertEqual(response["service"], "oj_admin")

        status, _, body = request(self.gateway, path="/api/questions?page=1&page_size=5")
        response = json.loads(body)
        self.assertEqual(status, 200)
        self.assertIsInstance(response["questions"], list)
        self.assertIsInstance(response["page"]["total_items"], str)

    def test_unknown_json_field_is_rejected_before_upstream(self):
        _, headers, body = request(self.gateway, path="/api/csrf")
        token = json.loads(body)["csrf_token"]
        cookie = next(value for name, value in headers if name.lower() == "set-cookie").split(";", 1)[0]
        request_headers = {
            "Content-Type": "application/json", "Origin": "https://localhost",
            "Cookie": cookie, "X-CSRF-Token": token,
        }
        status, _, response = request(self.gateway, "POST", "/api/auth/register",
                                      '{"unknown":true}', request_headers)
        self.assertEqual(status, 400)
        self.assertEqual(json.loads(response)["status"]["message"], "INVALID_JSON_REQUEST")

    def test_internal_rpc_name_cannot_be_addressed(self):
        self.assertEqual(request(self.gateway, "POST", "/api/UpdateJudgeResult", "{}",
                                 {"Content-Type": "application/json"})[0], 404)

    def test_every_declared_rpc_route_reaches_gateway_contract(self):
        tokens = {}
        for area, bootstrap in (("main", "/api/csrf"), ("admin", "/admin/api/csrf")):
            _, headers, body = request(self.gateway, path=bootstrap)
            token = json.loads(body)["csrf_token"]
            cookie = next(value for name, value in headers if name.lower() == "set-cookie").split(";", 1)[0]
            tokens[area] = token, cookie

        for route in parse_routes()[0]:
            path = route["path"]
            for parameter in ("uid", "question_id", "solution_id", "comment_id",
                              "submission_id", "admin_id", "test_case_id"):
                path = path.replace(f":{parameter}", "1")
            path = path.replace(":task_id", "phase8-task")
            token, cookie = tokens[route["area"]]
            headers = {
                "Content-Type": "application/json",
                "Origin": "https://localhost",
                "Cookie": cookie,
                "X-CSRF-Token": token,
            }
            body = None if route["verb"] == "GET" else "{}"
            status, _, response_body = request(self.gateway, route["verb"], path, body, headers)
            with self.subTest(method=route["method"], status=status):
                response = json.loads(response_body)
                self.assertNotEqual(response.get("status", {}).get("message"), "ROUTE_NOT_FOUND")
                self.assertNotEqual(response.get("status", {}).get("message"), "INVALID_UPSTREAM_RESPONSE")


if __name__ == "__main__":
    unittest.main()
