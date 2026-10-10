# PocketBox OpenWrt AP + Ethernet LAN profile (experimental)

This directory contains a **non-destructive profile generator**, not a flashable firmware image or an installer. It emits UCI commands for review. Nothing is applied automatically.

The intended topology is Wi-Fi AP + Ethernet LAN in one L2 bridge (`br-lan`), with PocketBox's own services on that LAN. External IIAB/BBS/NAS machines need no PocketBox-specific integration.

## Before use

1. Confirm the exact WR703N/Mango hardware revision, supported OpenWrt release, recovery path and backup of calibration partitions.
2. Verify the actual wired device and Wi-Fi radio using `ubus call system board`, `uci show network`, `uci show wireless`, and `ip link`. **Do not assume `eth0` or `radio0` is correct.**
3. Decide which host runs DHCP. The generator assumes **PocketBox runs DHCP** and an attached host uses a static address (for example 192.168.44.2/24).
4. Review the output before any manual changes. Keep a recovery console available: changing LAN addresses or wireless settings can disconnect management access.

## Generate reviewable commands (on a development machine)

```sh
python3 openwrt/generate_ap_lan.py --ethernet eth0 --radio radio0 --ssid PocketBox --password 'replace-with-a-strong-password'
```

The output is printed, **not executed**. Use the actual Ethernet and radio names from the target, not these examples. This generator assumes a modern OpenWrt `config device` bridge and may not apply to old WR703N-supported releases; port and syntax must be qualified first.

For an intentionally open offline hotspot, pass `--open` instead of a password. An open AP provides no link-layer privacy and must be used only in a trusted, isolated setting.

No WAN configuration is generated, and no default internet route is needed. The generated UCI changes are an illustrative starting point, not an approved production configuration.

## Acceptance checklist

- [ ] Wi-Fi client joins PocketBox SSID
- [ ] Wi-Fi client obtains one DHCP lease from the chosen DHCP authority
- [ ] Wi-Fi client reaches PocketBox's LAN address
- [ ] Wi-Fi client reaches HTTP and arbitrary TCP services on Ethernet host
- [ ] Disconnect Ethernet host; PocketBox local services remain available
- [ ] No unintended internet routing or second DHCP server
- [ ] Both WR703N and Mango tested with their **own** verified firmware and interface mapping
