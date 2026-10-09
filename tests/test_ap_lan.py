import subprocess
import sys
import unittest


class GeneratorTests(unittest.TestCase):
    def run_generator(self, *args):
        return subprocess.run(
            [sys.executable, "openwrt/generate_ap_lan.py", *args],
            capture_output=True, text=True
        )

    def test_bridge_ap_and_dhcp(self):
        result = self.run_generator("--ethernet", "eth0", "--radio", "radio0",
                                    "--ssid", "PocketBox", "--password", "test-password")
        self.assertEqual(result.returncode, 0, result.stderr)
        for expected in ("type=bridge", "ports=eth0", "network=lan", "mode=ap",
                         "encryption=psk2", "dhcp.lan.ignore=0"):
            self.assertIn(expected, result.stdout)
        self.assertNotIn("network.wan", result.stdout)
        self.assertNotIn("uci commit network\n", result.stdout)

    def test_reject_injection(self):
        result = self.run_generator("--ethernet", "eth0;reboot", "--radio", "radio0", "--open")
        self.assertNotEqual(result.returncode, 0)

    def test_open_ap_explicit(self):
        result = self.run_generator("--ethernet", "eth0", "--radio", "radio0", "--open")
        self.assertEqual(result.returncode, 0)
        self.assertIn("encryption=none", result.stdout)

    def test_password_required(self):
        result = self.run_generator("--ethernet", "eth0", "--radio", "radio0")
        self.assertNotEqual(result.returncode, 0)


if __name__ == "__main__":
    unittest.main()
