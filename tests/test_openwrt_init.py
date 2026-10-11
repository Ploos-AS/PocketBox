"""Static contract checks for the experimental OpenWrt procd script.

These are host-side checks, not execution against an OpenWrt runtime.
"""
import pathlib
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]
INIT = ROOT / "openwrt/package/pocketbox/files/pocketbox.init"
CONFIG = ROOT / "openwrt/package/pocketbox/files/pocketbox.config"


class OpenWrtInitContract(unittest.TestCase):
    def test_opt_in_and_loopback_defaults(self):
        config = CONFIG.read_text()
        self.assertIn("option enabled '0'", config)
        self.assertIn("option bind '127.0.0.1'", config)

    def test_procd_and_configured_bind(self):
        script = INIT.read_text()
        self.assertIn("USE_PROCD=1", script)
        self.assertIn("config_get_bool enabled main enabled 0", script)
        self.assertIn('config_get bind main bind 127.0.0.1', script)
        self.assertIn('command /usr/bin/pocketbox --bind "$bind"', script)
        self.assertIn('validate_port "$http_port"', script)
        self.assertIn('validate_port "$terminal_port"', script)

    def test_no_implicit_network_mutation(self):
        script = INIT.read_text()
        for forbidden in ("uci set", "uci commit", "ifconfig ", "ip addr add",
                          "iptables ", "nft ", "wifi reload", "network restart"):
            self.assertNotIn(forbidden, script)


if __name__ == "__main__":
    unittest.main()
