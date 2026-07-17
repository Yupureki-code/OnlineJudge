import unittest

from _contracts import parse_descriptor, parse_routes


class RouteContractTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.descriptor = parse_descriptor()
        cls.routes, cls.denied = parse_routes()

    def test_routes_are_unique_and_namespaced(self):
        keys = {(route["verb"], route["path"]) for route in self.routes}
        self.assertEqual(len(keys), len(self.routes))
        for route in self.routes:
            prefix = "/admin/api/" if route["area"] == "admin" else "/api/"
            self.assertTrue(route["path"].startswith(prefix), route)

    def test_route_types_match_descriptor(self):
        for route in self.routes:
            service = "oj.biz.OJAdminService" if route["area"] == "admin" else "oj.biz.OJService"
            expected = self.descriptor["services"][service][route["method"]]
            self.assertEqual(expected, (route["request"], route["response"]), route)

    def test_every_rpc_is_routed_or_explicitly_denied(self):
        routed = {
            ("oj.biz.OJAdminService" if route["area"] == "admin" else "oj.biz.OJService")
            + "." + route["method"] for route in self.routes
        }
        all_methods = {
            service + "." + method
            for service in ("oj.biz.OJService", "oj.biz.OJAdminService")
            for method in self.descriptor["services"][service]
        }
        self.assertEqual(all_methods, routed | set(self.denied))
        self.assertFalse(routed & set(self.denied))

    def test_internal_callbacks_are_denied(self):
        self.assertIn("oj.biz.OJService.UpdateJudgeResult", self.denied)
        self.assertIn("oj.biz.OJService.LegacyUpdateJudgeResult", self.denied)


if __name__ == "__main__":
    unittest.main()
