#!/usr/bin/env python3
"""Stage the PocketBox package into an existing OpenWrt source/build tree.

Does not build, install, flash, or alter router network settings.
"""
import argparse
from pathlib import Path
import shutil
import sys


def stage(source: Path, target: Path) -> None:
    source = source.resolve()
    target = target.resolve()
    package = source / "openwrt/package/pocketbox"
    required = [
        source / "src/main.c",
        source / "src/catalog.h",
        source / "src/http_async.h",
        package / "Makefile",
        package / "files/pocketbox.init",
        package / "files/pocketbox.config",
    ]
    for file in required:
        if not file.is_file():
            raise ValueError(f"missing required source: {file}")
    if not (target / "rules.mk").is_file() or not (target / "include/package.mk").is_file():
        raise ValueError("destination is not an OpenWrt source/build tree")
    destination = target / "package/pocketbox"
    if destination.exists():
        raise ValueError(f"destination already exists (refusing overwrite): {destination}")
    destination.mkdir(parents=True)
    try:
        shutil.copy2(package / "Makefile", destination / "Makefile")
        shutil.copytree(package / "files", destination / "files")
        (destination / "src").mkdir()
        for name in ("main.c", "catalog.h", "http_async.h"):
            shutil.copy2(source / "src" / name, destination / "src" / name)
    except Exception:
        shutil.rmtree(destination)
        raise
    print(f"Staged PocketBox at {destination}")
    print("Next: select package in menuconfig; make package/pocketbox/compile V=s")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--openwrt-tree", type=Path, required=True)
    args = parser.parse_args()
    try:
        stage(Path(__file__).resolve().parents[1], args.openwrt_tree)
    except (OSError, ValueError) as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
