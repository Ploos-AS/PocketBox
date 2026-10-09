# Roadmap

- **M0 (current):** Document architecture, define WR703N/Mango constraints, C11 host prototype for HTTP + ASCII TCP terminal, host CI and smoke tests. No device claims.
- **M1:** Shared catalog layer, robust C parser, bounded asynchronous poll-loop and file/connection limits.
- **M2:** WR703N and Mango OpenWrt toolchains, USB mounting, repeatable package/image builds and hardware test evidence.
- **M3:** Genuine Telnet IAC negotiation, ANSI 80x25, shared library navigation and text search.
- **M4:** Controlled uploads, storage quota, atomic metadata updates, audit trail, safe public LAN exposure.
- **M5:** Offline EPUB/PDF catalog, file-area exports (FILES.BBS, Aminet-style .readme), chat/message areas.
- **M6:** Gopher/FTP evaluation and optional retro terminal profiles (PETSCII), optional peer-to-peer sync.
- **v0.1.0:** Security and recovery reviewed images, WR703N and Mango acceptance tests, reproducible builds.

## Internet-in-a-Box (IIAB) Ethernet integration

**Accepted feature request (design only, not implemented).** PocketBox must optionally expose services of an **external** Internet-in-a-Box server connected to its Ethernet interface, while continuing to serve its own HTTP and Telnet/BBS interfaces over local Wi-Fi. No IIAB application stack is hosted on the MIPS router.

- **WR703N:** single Ethernet port, selectable IIAB uplink; **Mango:** configurable Ethernet port assignment, without assuming default LAN/WAN roles.
- **Phase 1:** routed, isolated Wi-Fi subnet with IIAB reachable via explicitly configured IP/hostname; PocketBox DHCP/DNS must not conflict with IIAB's DHCP/DNS. No automatic NAT/internet forwarding. Local portal links to IIAB's known services; avoid assuming fixed Kiwix/Kolibri ports.
- **Phase 2:** optional layer-2 bridge where Wi-Fi/driver/firmware and IIAB network settings allow it; document Wi-Fi AP bridging and failure modes. Choose one DHCP authority per segment.
- **Security:** prevent unintended internet/private-LAN access, disable administrative proxying, reject arbitrary proxy destinations (SSRF), keep Telnet on isolated network, and ensure local portal works when IIAB is offline.
- **Validation:** DHCP/DNS coexistence, IIAB IP discovery/manual config, offline portal links, Wi-Fi clients reaching IIAB, Ethernet disconnect/reconnect, port assignment, no unwanted uplink route, WR703N/Mango hardware tests.

**Schedule:** M1 design/config schema, M2 OpenWrt network integration, M3 portal integration and hardware qualification. No firmware or feature-complete implementation in M0.
