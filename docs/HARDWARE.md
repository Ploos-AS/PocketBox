# Hardware qualification

| Hardware | SoC | RAM | Flash | Qualification |
| --- | --- | --- | --- | --- |
| TP-Link TL-WR703N | Atheros AR9331 | 32 MiB | 4 MiB | Pending |
| GL.iNet GL-MT300N-V2 Mango | MediaTek MT7628 | 128 MiB | 16 MiB | Pending |

Both are MIPS, but use different OpenWrt targets and toolchains. Do not assume one image fits both. Verify board revisions, original firmware, firmware recovery paths, backup of calibration partitions, USB mass storage, kernel packages and available free space before any flashing.

M0 only validates host builds. No device firmware, cross-compiled binary, flash image or hardware test has been certified.

Target measurements for later releases: stripped binary size, peak RSS, CPU utilization, concurrent connections, USB throughput, file corruption under reboot/power-loss and recovery procedure success. For WR703N consider USB extroot only after measuring stock flash limits.
