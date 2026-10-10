# Cross-compiling PocketBox for OpenWrt (M1 preparation)

**No firmware image or tested OpenWrt package is supplied yet.** These are
instructions for building the userspace executable with an existing,
device-compatible OpenWrt SDK.

## Supported hardware only

| Device | CPU family | Flash | RAM |
| --- | --- | --- | --- |
| TP-Link TL-WR703N | Atheros AR9331 / MIPS 24K | 4 MiB | 32 MiB |
| GL.iNet Mango GL-MT300N-V2 | MediaTek MT7628 / MIPS 24K | 16 MiB | 128 MiB |

The devices require different OpenWrt targets and toolchains. **Do not use
a Mango image or package on WR703N or vice versa.** Confirm the hardware
revision, exact OpenWrt target/subtarget, SDK compatibility, flash space and
recovery method before building or installing anything. In particular,
current OpenWrt releases may not support a 4 MiB WR703N configuration.

## Build with a verified SDK

On a Linux build host, extract the SDK matching the chosen device's
installed OpenWrt release. Within the SDK, locate its MIPS musl cross-compiler
and add the toolchain bin directory to PATH. Then from the PocketBox repo:

```sh
make clean
make CC=mips-openwrt-linux-musl-gcc
file pocketbox
```

The compiler prefix above is **illustrative**; inspect the actual SDK
toolchain and use its exact compiler name. Do not substitute the host
compiler. The resulting binary must be MIPS for the correct target ABI,
not x86_64 or ARM. This step produces only a userspace binary, not an
`.ipk` or flashable firmware image.

## Qualification gates

1. Confirm both targets' supported OpenWrt releases and available SDKs.
2. Cross-compile and inspect ELF architecture, ABI, dynamic loader and size.
3. Check free flash after base system and Wi-Fi/AP dependencies.
4. Verify service binding, init integration and file root on the device.
5. Measure RSS, CPU, and file listing latency with USB storage attached.
6. Exercise HTTP, terminal, bridge, and DHCP with real devices.
7. Test recovery procedure before distributing firmware.

PocketBox firmware remains deliberately limited to local file services
and the AP/LAN bridge. IIAB, Kiwix, Kolibri and similar services belong on
a separate Ethernet-connected host.
