# Architecture (M0)

PocketBox is a small C11/POSIX program with HTTP and ASCII terminal adapters over a common file root. A future service layer will centralize file areas, metadata, library catalog, message boards and configuration, independent of protocol.

M0 is an intentionally limited feasibility prototype. It uses one sequential accept loop; a slow client can block other clients for the configured socket timeout. M1 replaces this with bounded, nonblocking `poll()` and per-client state. HTTP is currently GET-only and supports the homepage plus one-level basename downloads. Telnet currently exposes plain ASCII over a TCP socket, not a fully negotiated Telnet implementation. Never describe this as complete Telnet support.

### Constraints

- C11, OpenWrt-compatible POSIX calls, no heap-heavy libraries.
- WR703N is the strict lower bound; Mango may enable optional features.
- USB storage holds public library content; no writable public upload endpoints in M0.
- Default bind: localhost only. Network binding requires a security review.
- No ARM support or dependency on cloud services.

### Planned adapters

HTTP, RFC-aware Telnet/ANSI, optional FTP/Gopher and BBS file-area exports. Protocol services share file IDs, metadata and access policy.
