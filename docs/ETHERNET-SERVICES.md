# Ethernet Services: external offline service hosts

Status: **design proposal**, not implemented. Applies only to TL-WR703N and GL-MT300N-V2 Mango.

## Goal

PocketBox remains an independent offline-first C11 HTTP + Telnet/BBS appliance. Its Ethernet interface may connect to a separate, more capable offline host (Internet-in-a-Box, NAS, retro BBS, educational server, etc.). PocketBox makes curated services discoverable to Wi-Fi clients without requiring an internet connection. Ethernet is **not assumed to be a WAN uplink**.

## Integration tiers

1. **Link:** show curated local URLs/addresses in the HTTP portal and terminal menus; PocketBox does not proxy content.
2. **Reach:** allow carefully controlled Wi-Fi-to-Ethernet traffic using OpenWrt network configuration and firewall rules. No implicit internet access or arbitrary forwarding.
3. **Catalog:** optional read-only adapter to import safe metadata from an external service into PocketBox's own library/BBS file listings. No automatic mirroring of bulk content.
4. **Proxy (later):** only narrowly scoped, authenticated/allowlisted reverse proxy for specific HTTP services if a real need is proven; prevent SSRF and open-proxy behavior.

## Candidate external services

| Service | Examples | First integration |
| --- | --- | --- |
| Offline encyclopedia | Internet-in-a-Box, Kiwix/ZIM | Link + reach |
| Education | Kolibri, static course server | Link + reach |
| Retro file archive | FTP/HTTP archive, FILES.BBS, Aminet metadata | Link + catalog |
| Retro BBS | Mystic, Synchronet, Enigma½ | Reach + terminal link |
| Storage | NAS with HTTP/FTP/SMB/NFS | HTTP link, metadata adapter later |
| Communication | IRC, XMPP, local message board, Meshtastic bridge | Reach with explicit ports |
| Documentation/code | Forgejo, static manuals, Git mirrors | Link + reach |
| Offline maps | OpenStreetMap tile/map service | Link + reach |
| Books | Calibre-Web, Project Gutenberg mirror | Link + catalog |
| Gopher | Gopher service | Terminal links and reach |

PocketBox is not expected to execute these services on the 4 MiB flash WR703N.

## Networking modes

- **Default: routed isolated mode.** PocketBox Wi-Fi is its own subnet. Ethernet attaches to a designated offline service network. Firewall allows only configured destination addresses/ports. PocketBox controls Wi-Fi DHCP; upstream DHCP/DNS must not conflict. No automatic default route to internet.
- **Optional: bridged mode.** For suitable OpenWrt/driver/AP configurations, bridge Wi-Fi clients to Ethernet. Ensure exactly one DHCP server per broadcast domain. Requires hardware validation.
- **Offline fallback.** PocketBox HTTP/Telnet and local USB files continue operating when Ethernet host is disconnected.
- **WR703N:** one physical Ethernet port. **Mango:** distinguish its two Ethernet ports and physical roles through device-specific configuration; do not assume WAN/LAN assignments.

## Suggested config model (illustrative, not parsed yet)

```ini
[ethernet]
mode = routed
service_host = 192.168.77.2
allow_internet = false

[service.iiab]
type = web
title = Internet-in-a-Box
url = http://192.168.77.2/

[service.bbs]
type = telnet
title = Retro BBS
host = 192.168.77.2
port = 2324
```

The service address, port, and path are examples, not IIAB defaults. Configuration must validate addresses, URL schemes, ports and titles; never allow arbitrary proxy destinations.

## Acceptance criteria

- Service catalog available via HTTP and text terminal menus.
- USB library and local portal work when Ethernet is disconnected.
- Wi-Fi clients reach only configured offline host services; no unintentional private LAN or internet transit.
- DHCP/DNS coexistence tested, including host reboots and reconnections.
- Resource consumption measured on both MIPS targets.
- Never expose administrative UIs or credentials through Telnet.
- No claims of hardware support before physical testing.

## Milestones

M1: service catalog schema and static links in shared model; M2: OpenWrt network modes and firewall configuration; M3: HTTP/Telnet presentation and physical acceptance tests; later: protocol-aware metadata adapters.
