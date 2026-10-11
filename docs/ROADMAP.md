# PocketBox roadmap

- **M0 (prototype):** C11 host HTTP and ASCII-over-TCP terminal prototype, host CI, hardware constraints and architecture.
- **M1 (in progress):** Shared file catalog, bounded multi-client server, robust HTTP/Telnet parsers. **Wi-Fi AP + Ethernet LAN bridge configuration prototype** and host-side tests; no router changes yet.
- **M2:** OpenWrt toolchain and firmware/packaging qualification for WR703N and Mango; hardware-verified AP/LAN bridging, DHCP and service reachability. Preserve recovery paths and calibration data.
- **M3:** Real Telnet IAC/ANSI support, shared library navigation, search.
- **M4:** Safe uploads, quotas, atomic metadata, network hardening.
- **M5:** Offline book/retro file archives, FILES.BBS, message boards.
- **M6:** Optional Gopher/FTP, PETSCII, synchronization.
- **v0.1.0:** Device-tested, security-reviewed, reproducible releases.

## Ethernet decision

**PocketBox is a Wi-Fi AP, not an IIAB proxy.** Wi-Fi clients must be able to reach arbitrary services on a computer connected to PocketBox's Ethernet LAN (IIAB, Kiwix, NAS, BBS, SSH, IRC, etc.). Default design: same L2 LAN bridge with one DHCP authority. No application-specific integrations required. PocketBox's own HTTP/Telnet and USB files work without the external host. Routed/isolation mode and curated portal links are optional later. See [Ethernet services](ETHERNET-SERVICES.md) and [OpenWrt profile](../openwrt/README.md).

M1 provides a **review-only UCI command generator**, not an installer or validated router configuration. Physical tests remain required.
