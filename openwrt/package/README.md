# Experimental OpenWrt package scaffold

This is a **developer-only package scaffold**, not a qualified router image.
PocketBox binds to **127.0.0.1 by default**. To expose the service to LAN
clients, configure an explicit IPv4 address assigned to the PocketBox LAN
interface. Wildcard binding (0.0.0.0) is rejected. Review firewall rules
and verify the actual LAN address before enabling the service.

To stage the package in a compatible, already configured OpenWrt build tree:

```sh
mkdir -p /path/to/openwrt/package/pocketbox/src
cp -R openwrt/package/pocketbox/Makefile openwrt/package/pocketbox/files /path/to/openwrt/package/pocketbox/
cp src/*.c src/*.h /path/to/openwrt/package/pocketbox/src/
```

From that OpenWrt build tree, select the package using menuconfig, then run
`make package/pocketbox/compile V=s`. Verify target architecture and package
size. Do not flash an image until the exact device/revision is qualified.

The init service is disabled by default. It does not modify firewall, DHCP,
Wi-Fi, bridge, or other network configuration. The configured file root must
already exist. USB mounting is managed independently by the administrator.

The package has **not** been cross-compiled or tested on physical hardware.
