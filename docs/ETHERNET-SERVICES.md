# Ethernet connectivity: PocketBox as Wi-Fi access point

Status: architecture decision; hardware implementation pending.

## Core requirement

**PocketBox is the Wi-Fi access point.** Wi-Fi clients should be able to reach services running on a separate computer connected to PocketBox's Ethernet port. PocketBox does **not** need to understand, enumerate, proxy, or integrate those services. This applies to Internet-in-a-Box, Kiwix, Kolibri, BBS servers, NAS, IRC, HTTP, SSH and arbitrary other services offered by the attached host.

PocketBox's own C11 HTTP portal, local USB library and Telnet/BBS continue to operate independently. Service discovery, catalogs, application-level adapters and proxies are optional future enhancements, **not** requirements for basic Ethernet connectivity.

## Preferred topology: simple LAN bridge

```text
  Wi-Fi clients ))) PocketBox (Wi-Fi AP) --- Ethernet --- external host
                        |
                 local PocketBox services
```

- **Default design target:** bridge Wi-Fi AP interface and Ethernet LAN interface into one Layer-2 LAN using OpenWrt network configuration. This makes arbitrary Ethernet-host services reachable without PocketBox-specific application code.
- **WR703N:** single Ethernet port used as LAN to the external host.
- **Mango:** designate at least one Ethernet port as LAN, retaining the ability to configure the other separately; verify actual OpenWrt interface mappings.
- **IP addresses:** either PocketBox runs DHCP for the bridged LAN and the external host uses a static/reserved address, or a deliberate alternative DHCP authority is selected. Never run two conflicting DHCP servers.
- **DNS:** optional local name for the external host; direct IP access must work without DNS.
- **No WAN/internet required:** no default internet uplink, captive portal or proxy required. Do not hijack arbitrary DNS/HTTP traffic.
- **Isolation:** do not bridge into an unknown or untrusted external LAN without reviewing the consequences; Wi-Fi clients can directly access exposed Ethernet-host services. Consider firewalling or a routed alternative for untrusted deployments.

## Alternative topology: routed AP

Where bridging is unavailable or isolation is required, place Ethernet and Wi-Fi on separate subnets and route between them using OpenWrt. The external host needs a return route or an explicitly configured NAT policy. This is a secondary, opt-in mode, not the default design target.

## M0/M1 boundaries

M0: architecture and host prototype; no real device network changes. M1/M2: implement OpenWrt AP+LAN bridge profiles for WR703N and Mango, confirm correct Ethernet interfaces, DHCP behavior and basic reachability using arbitrary TCP services. Verify hardware revisions and firmware/recovery before flashing.

## Acceptance tests

1. Wi-Fi client associates to PocketBox and receives a valid IP address.
2. Client reaches an HTTP service on Ethernet-connected computer by IP.
3. Client reaches arbitrary TCP service (e.g. SSH or Telnet) on the same host without PocketBox code changes.
4. PocketBox local HTTP/Telnet and USB library remain accessible.
5. Unplugging Ethernet does not break PocketBox's own Wi-Fi services.
6. No unintended internet connectivity, rogue DHCP, or DNS redirection.

Optional links to IIAB or BBS in the PocketBox portal may be added later, but are not required for any of the above.
