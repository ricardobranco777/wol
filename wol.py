#!/usr/bin/env python3
"""
Wake on LAN
"""

import re
import socket
import struct
import sys


MAC_REGEX = r"^(?:[0-9A-Fa-f]{2}[:-]?){5}(?:[0-9A-Fa-f]{2})$"


def wake_on_lan(mac_address: str) -> None:
    """
    Wake on LAN
    """
    payload = "FF" * 6 + mac_address * 16
    packet = b"".join(
        struct.pack("B", int(payload[i : i + 2], 16)) for i in range(0, len(payload), 2)
    )
    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as sock:
        sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
        for _ in range(10):
            try:
                sock.sendto(packet, ("255.255.255.255", 9))
            except OSError as exc:
                sys.exit(f"{exc}")


def main() -> None:
    """
    Main function
    """
    for arg in sys.argv[1:]:
        if not re.search(MAC_REGEX, arg):
            sys.exit(f"Invalid MAC: {arg}")
        mac_address = arg.replace(":", "").replace("-", "")
        wake_on_lan(mac_address)


if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        sys.exit(1)
