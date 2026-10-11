# Security model

PocketBox is designed for local/offline operation, **not anonymous or automatically secure networking**. Original WR703N-era firmware is obsolete. Never connect a legacy build directly to the public internet. M0 listens only on 127.0.0.1, does not implement authentication, and is not an appliance firmware release.

M0 file downloads use basename-only paths relative to a pre-opened directory with O_NOFOLLOW; uploaded content and symlinks are out of scope. Before any exposure beyond loopback, implement URL normalization and protocol parsing limits, nonblocking clients, idle timeouts, backpressure, safe error handling, and fuzz tests. Reject percent-encoded path components rather than ambiguously decoding them. Admin access must be separate from public Telnet and HTTP. No secrets/passwords over Telnet. Future uploads require per-file and total quotas, atomic writes and safe names. Block bridging to private LAN and internet by default.
