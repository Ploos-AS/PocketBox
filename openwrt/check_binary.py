#!/usr/bin/env python3
"""Inspect a PocketBox ELF binary without running it.

A passing check does not imply firmware/device compatibility.
"""
import argparse
from pathlib import Path
import struct
import sys

ELF_MACHINE_MIPS = 8


def inspect(path: Path) -> dict:
    with path.open("rb") as stream:
        header = stream.read(20)
    if len(header) < 20 or header[:4] != b"\x7fELF":
        raise ValueError("not an ELF binary")
    elf_class, data = header[4], header[5]
    if elf_class not in (1, 2) or data not in (1, 2):
        raise ValueError("invalid ELF class or byte order")
    order = "<" if data == 1 else ">"
    machine = struct.unpack_from(order + "H", header, 18)[0]
    return {
        "bytes": path.stat().st_size,
        "elf_bits": 32 if elf_class == 1 else 64,
        "endian": "little" if data == 1 else "big",
        "machine": machine,
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("binary", type=Path)
    parser.add_argument("--target", choices=("mango", "wr703n"), required=True)
    args = parser.parse_args()
    try:
        result = inspect(args.binary)
    except (OSError, ValueError) as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1
    print(f"Target: {args.target}")
    for key, value in result.items():
        print(f"{key}: {value}")
    if result["machine"] != ELF_MACHINE_MIPS or result["elf_bits"] != 32:
        print("error: expected a 32-bit MIPS ELF, not a host binary", file=sys.stderr)
        return 1
    expected_endian = "little" if args.target == "mango" else "big"
    if result["endian"] != expected_endian:
        print(f"error: expected {expected_endian}-endian MIPS for {args.target}",
              file=sys.stderr)
        return 1
    print("ELF architecture matches expected family; ABI, libraries, flash and hardware still require validation.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
