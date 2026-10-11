#!/usr/bin/env python3
"""Emit (never execute) reviewable OpenWrt UCI AP+Ethernet bridge commands."""
import argparse
import re
import shlex

IDENT = re.compile(r"^[A-Za-z0-9_.-]{1,32}$")


def valid_ident(value):
    if not IDENT.fullmatch(value):
        raise argparse.ArgumentTypeError("must be a device identifier (1-32 letters/digits/._-)")
    return value


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--ethernet", required=True, type=valid_ident)
    parser.add_argument("--radio", required=True, type=valid_ident)
    parser.add_argument("--ssid", default="PocketBox")
    security = parser.add_mutually_exclusive_group(required=True)
    security.add_argument("--password")
    security.add_argument("--open", action="store_true")
    parser.add_argument("--address", default="192.168.44.1")
    args = parser.parse_args()
    if not (1 <= len(args.ssid.encode("utf-8")) <= 32) or any(ord(c) < 32 for c in args.ssid):
        parser.error("SSID must be 1-32 UTF-8 bytes without control characters")
    if args.password and not (8 <= len(args.password) <= 63):
        parser.error("WPA2 passphrase must be 8-63 characters")
    if args.password and (not args.password.isascii() or not args.password.isprintable()):
        parser.error("use a printable ASCII WPA2 passphrase")
    if not args.open and not args.password:
        parser.error("provide --password or --open")
    import ipaddress
    try:
        ip = ipaddress.IPv4Address(args.address)
        if not ip.is_private or ip.is_multicast or ip.is_unspecified:
            raise ValueError()
    except ValueError:
        parser.error("address must be a private IPv4 address")
    q = shlex.quote
    lines = [
        "# REVIEW ONLY. Not executed. BACK UP UCI settings and verify interfaces first.",
        "# WARNING: changes to LAN/Wi-Fi may disconnect management access.",
        "uci -q delete network.pocketbox_bridge",
        "uci set network.pocketbox_bridge=device",
        "uci set network.pocketbox_bridge.name=br-lan",
        "uci set network.pocketbox_bridge.type=bridge",
        f"uci add_list network.pocketbox_bridge.ports={q(args.ethernet)}",
        "uci set network.lan=interface",
        "uci set network.lan.device=br-lan",
        "uci set network.lan.proto=static",
        f"uci set network.lan.ipaddr={q(str(ip))}",
        "uci set network.lan.netmask=255.255.255.0",
        "uci set dhcp.lan=dhcp",
        "uci set dhcp.lan.interface=lan",
        "uci set dhcp.lan.start=100",
        "uci set dhcp.lan.limit=100",
        "uci set dhcp.lan.leasetime=12h",
        "uci set dhcp.lan.ignore=0",
        "uci -q delete wireless.pocketbox_ap",
        "uci set wireless.pocketbox_ap=wifi-iface",
        f"uci set wireless.pocketbox_ap.device={q(args.radio)}",
        "uci set wireless.pocketbox_ap.mode=ap",
        "uci set wireless.pocketbox_ap.network=lan",
        f"uci set wireless.pocketbox_ap.ssid={q(args.ssid)}",
        f"uci set wireless.pocketbox_ap.encryption={'none' if args.open else 'psk2'}",
    ]
    if args.password:
        lines.append(f"uci set wireless.pocketbox_ap.key={q(args.password)}")
    lines += [
        "# DO NOT blindly paste this output: inspect existing bridge, LAN, DHCP and wireless sections.",
        "# Existing LAN devices/ports may already be in br-lan; merging is device-specific.",
        "# If reviewed and applied manually: uci commit network; uci commit wireless; uci commit dhcp",
        "# Then arrange a safe rollback before reloading network/wifi/dnsmasq.",
    ]
    print("\n".join(lines))


if __name__ == "__main__":
    main()
