# wol
Wake on LAN

## Usage

```
usage: ./wol [-i interface] lladdr...
```

Tested on:
- Linux
- Illumos
- FreeBSD
- NetBSD
- OpenBSD
- DragonflyBSD
- MidnightBSD

# Notes

- `lladdr` can be a MAC address or an entry in [/etc/ethers](https://linux.die.net/man/5/ethers)
- FreeBSD has [wake](https://man.freebsd.org/cgi/man.cgi?query=wake) but needs access to `/dev/bpf`
- Ditto above for NetBSD with [wakeonlan](https://man.netbsd.org/wakeonlan.8)
