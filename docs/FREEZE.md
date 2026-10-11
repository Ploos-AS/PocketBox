# PocketBox foundation freeze

Status: **FROZEN — awaiting MikroWrt integration contract**
Date: 2026-10-11
Branch: `m0-foundation`
Review: PR #1 (leave unmerged)

## Scope

Preserve the C11 HTTP file portal, ASCII terminal, catalog, bounded clients,
security checks, host tests and experimental packaging as a reference baseline.
The generic MIPS little-endian cross-build is a portability smoke test only;
it is not an installable MikroWrt/OpenWrt package.

## Allowed during freeze

- Fix verified correctness or security defects.
- Repair CI failures and maintain tests/documentation.
- Record integration requirements and measurements without redesigning runtime.

## Deferred until MikroWrt contracts are established

- Refactor runtime and service integration.
- Select package manager, package metadata and dependency policy.
- Bind to MikroWrt init/service supervision and configuration.
- Define AP/LAN/USB storage ownership and lifecycle.
- Build or flash device firmware, or claim Mango/WR703N qualification.

## Unfreeze gates

1. MikroWrt establishes supported device targets and reproducible toolchains.
2. Package format and installation/update policy are defined.
3. Init/service supervision and configuration conventions are defined.
4. Networking ownership is explicit: PocketBox does not mutate AP, bridge,
   DHCP, firewall or upstream host services.
5. USB mount and storage paths are defined with safe failure behavior.
6. A migration plan preserves HTTP/terminal behavior and regression tests.

Do not merge PR #1 as part of the freeze. Revisit this document before
starting integration or refactoring.
