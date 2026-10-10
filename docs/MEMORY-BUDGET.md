# PocketBox M1 memory budget

The TL-WR703N (32 MiB RAM, 4 MiB flash) is the limiting target. These are
source-level bounds, not measured device RAM or firmware sizes.

| Item | Bound |
| --- | ---: |
| Terminal sessions | 12 |
| Terminal queued output per session | 4,608 bytes |
| Total terminal output buffers | 55,296 bytes |
| Previous total (8,192 bytes each) | 98,304 bytes |
| Reduction | 43,008 bytes |
| Catalog entries per page | 32 |
| Catalog filename bytes | 128 |
| Catalog names array | 4,096 bytes |
| HTTP sessions | 8 |

A full terminal catalog page of 32 names of at most 127 bytes plus CRLF,
heading, and menu fits inside 4,608 bytes. Multiple requests from a client
that does not read responses may exhaust its queue; PocketBox disconnects
that client rather than dropping bytes silently.

The C compiler and runtime may add padding, stack, socket buffers and other
overheads. **Do not treat these figures as measured RSS or flash usage.**

## Before hardware release

- Measure binary size and RSS with the intended OpenWrt musl toolchains.
- Verify 4 MiB flash layout and package fit on TL-WR703N.
- Load-test concurrent HTTP and terminal clients on both supported devices.
- Check catalog HTML pagination with 127-character names; a truncated HTML
  page must never skip files when advancing the cursor.
- Record available RAM after Wi-Fi AP, bridge and DHCP are active.
