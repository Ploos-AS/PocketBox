# PocketBox

Offline-first portable library and BBS-inspired service for **TP-Link TL-WR703N** and **GL.iNet GL-MT300N-V2 (Mango)**.

**M0:** experimental host-buildable C11 proof of concept (HTTP homepage/file download and ASCII-over-TCP terminal menu). Not yet OpenWrt device-qualified, production hardened, or a complete Telnet implementation.

## Quick start (Linux)

```sh
make
mkdir -p demo-files
printf 'PocketBox offline library\n' > demo-files/welcome.txt
./pocketbox --root demo-files --http-port 8080 --telnet-port 2323
# In another terminal:
curl http://127.0.0.1:8080/
curl http://127.0.0.1:8080/files/welcome.txt
telnet 127.0.0.1 2323
make test
```

The prototype binds to **127.0.0.1 only**. This is **not** a network appliance release. It cannot yet be installed directly as firmware.

## Hardware scope

| Target | Processor | RAM | Flash | Qualification |
| --- | --- | --- | --- | --- |
| TL-WR703N | AR9331 MIPS | 32 MiB | 4 MiB | Pending |
| GL-MT300N-V2 | MT7628 MIPS | 128 MiB | 16 MiB | Pending |

No ARM or x86 hardware target is planned. Verify hardware revisions, OpenWrt support, backup and recovery paths, USB storage support and RAM/flash budgets **before flashing**. No firmware images are provided by M0.

## Documentation

- [Architecture](docs/ARCHITECTURE.md)
- [Hardware qualification](docs/HARDWARE.md)
- [Roadmap](docs/ROADMAP.md)
- [Security model](docs/SECURITY.md)

MIT licensed, copyright Ploos AS.
