import subprocess
import unittest

from _contracts import ROOT, parse_descriptor


class DescriptorContractTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        subprocess.run([str(ROOT / "scripts/generate_gateway_descriptor.sh")], check=True)
        cls.contract = parse_descriptor()

    def test_required_services_exist(self):
        self.assertIn("oj.biz.OJService", self.contract["services"])
        self.assertIn("oj.biz.OJAdminService", self.contract["services"])

    def test_imported_messages_exist(self):
        for message in ("oj.common.EmptyRequest", "oj.common.StatusResponse",
                        "oj.biz.CreateSubmissionRequest", "oj.biz.AdminLoginResponse"):
            self.assertIn(message, self.contract["messages"])

    def test_descriptor_is_import_complete(self):
        names = {name for name in self.contract["services"]}
        self.assertEqual(names, {"oj.biz.OJService", "oj.biz.OJAdminService", "oj_judge.JudgeService"})


if __name__ == "__main__":
    unittest.main()
