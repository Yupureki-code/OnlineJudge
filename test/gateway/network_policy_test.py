import json
import subprocess
import unittest

from _contracts import ROOT


class NetworkPolicyTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        output = subprocess.check_output([
            "docker", "compose", "--env-file", str(ROOT / "docker/compose.dev.env"),
            "-f", str(ROOT / "docker/docker-compose.yml"),
            "config", "--format", "json",
        ], text=True)
        cls.config = json.loads(output)

    def test_only_gateway_publishes_ports(self):
        for name, service in self.config["services"].items():
            ports = service.get("ports", [])
            if name == "openresty":
                self.assertEqual({str(port["published"]) for port in ports}, {"80", "443"})
            else:
                self.assertEqual(ports, [], name)

    def test_internal_network_and_backend_hardening(self):
        self.assertTrue(self.config["networks"]["internal"]["internal"])
        for name in ("oj_server", "oj_admin", "judge_service"):
            service = self.config["services"][name]
            self.assertTrue(service["read_only"])
            self.assertIn("ALL", service["cap_drop"])

    def test_running_backends_have_no_host_port_bindings(self):
        for service in ("oj_server", "oj_admin", "judge_service", "mysql", "redis", "rabbitmq"):
            container_id = subprocess.run([
                "docker", "compose", "--env-file", str(ROOT / "docker/compose.dev.env"),
                "-f", str(ROOT / "docker/docker-compose.yml"),
                "ps", "-q", service,
            ], text=True, capture_output=True, check=True).stdout.strip()
            if not container_id:
                continue
            bindings = json.loads(subprocess.check_output([
                "docker", "inspect", "--format", "{{json .HostConfig.PortBindings}}", container_id,
            ], text=True))
            self.assertFalse(bindings, service)


if __name__ == "__main__":
    unittest.main()
