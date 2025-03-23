#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <ifaddrs.h>
#if defined(__linux__)
#include <netinet/ether.h>
#elif defined(__FreeBSD__) || defined(__DragonFly__)
#include <net/ethernet.h>
#define	ether_addr_octet octet
#elif defined(__sun__)
#include <sys/ethernet.h>
#include <sys/sockio.h>
#elif defined(__NetBSD__)
#include <net/if_ether.h>
#elif defined(__OpenBSD__)
#include <netinet/if_ether.h>
#else
#error not supported
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <err.h>
#include <getopt.h>

#ifndef ETHER_ADDR_LEN
#define ETHER_ADDR_LEN	6
#endif

#ifdef __linux__
extern char *__progname;
#define getprogname()   (__progname)
#endif

#define USAGE "%s [-i interface] [-P port] [-p password] lladdr..."

static in_addr_t
get_broadcast_address(const char *ifname)
{
	struct ifreq ifr;
	int sock;

	if (ifname == NULL || ifname[0] == '\0')
		return INADDR_BROADCAST;

	if ((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
		err(1, "socket");

	memset(&ifr, 0, sizeof(ifr));
	strncpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name));

	if (ioctl(sock, SIOCGIFBRDADDR, &ifr) == -1)
		err(1, "ioctl: %s", ifname);

	(void)close(sock);
	return ((struct sockaddr_in *)&ifr.ifr_broadaddr)->sin_addr.s_addr;
}

static in_addr_t
get_broadcast_address2(const char *ifaddr, const struct ifaddrs *ifaddrs)
{
	unsigned int valid_flags = IFF_UP | IFF_RUNNING | IFF_BROADCAST;
	const struct ifaddrs *ifa;
	struct in_addr addr;
	int rc;

	if ((rc = inet_pton(AF_INET, ifaddr, &addr)) < 0)
		err(1, "inet_pton: %s", ifaddr);
	else if (!rc)
		errx(1, "inet_pton: %s: invalid address", ifaddr);

	for (ifa = ifaddrs; ifa != NULL; ifa = ifa->ifa_next) {
		if (ifa->ifa_addr->sa_family != AF_INET ||
		    (ifa->ifa_flags & valid_flags) != valid_flags ||
		    ifa->ifa_broadaddr == NULL)
			continue;
		if (((struct sockaddr_in *)ifa->ifa_addr)->sin_addr.s_addr == addr.s_addr)
			return ((struct sockaddr_in *)ifa->ifa_broadaddr)->sin_addr.s_addr;
	}

	return INADDR_BROADCAST;
}

static uint8_t *
get_password(const char *str)
{
	static uint8_t password[ETHER_ADDR_LEN];
	unsigned int values[ETHER_ADDR_LEN];
	int i;

	if (sscanf(str, "%2x:%2x:%2x:%2x:%2x:%2x",
	   &values[0], &values[1], &values[2],
	   &values[3], &values[4], &values[5]) != ETHER_ADDR_LEN)
		return NULL;

	for (i = 0; i < ETHER_ADDR_LEN; i++)
		password[i] = (uint8_t)values[i];

	return password;
}

static void
wake(const struct ether_addr *mac, struct sockaddr_in sin, const uint8_t *password) {
	uint8_t payload[102 + ETHER_ADDR_LEN];
	char addrstr[INET_ADDRSTRLEN];
	size_t size;
	int on = 1;
	int sock;

	memset(payload, 0xFF, ETHER_ADDR_LEN);
	for (size = ETHER_ADDR_LEN; size < sizeof(payload); size += ETHER_ADDR_LEN)
		memcpy(payload + size, mac->ether_addr_octet, ETHER_ADDR_LEN);

	if (password != NULL) {
		memcpy(payload + 102, password, ETHER_ADDR_LEN);
		size += ETHER_ADDR_LEN;
	}

	if ((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
		err(1, "socket");

	if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &on, sizeof(int)) == -1)
		err(1, "setsockopt");

	if (inet_ntop(AF_INET, &sin.sin_addr, addrstr, sizeof(addrstr)) == NULL)
		err(1, "inet_ntop");

	printf("Sending to %s via %s\n", ether_ntoa(mac), addrstr);

	if (sendto(sock, payload, size, 0, (struct sockaddr*)&sin, sizeof(sin)) == -1)
		err(1, "sendto");

	(void)close(sock);
}

int
main(int argc, char *argv[]) {
	char ifname[IF_NAMESIZE] = {0};
	struct ether_addr ea, *mac;
	uint8_t *password = NULL;
	struct ifaddrs *ifaddrs;
	struct sockaddr_in sin;
	uint16_t port = 9;
	int i, opt;

	while ((opt = getopt(argc, argv, "i:P:p:")) != -1) {
		switch (opt) {
		case 'i':
			strncpy(ifname, optarg, sizeof(ifname) - 1);
			break;
		case 'P': {
			char *endptr;
			unsigned long n = strtoul(optarg, &endptr, 10);
			if (*endptr != '\0' || n > 65535)
				errx(1, "Invalid port: %s", optarg);
			port = (uint16_t)n;
			break;
		}
		case 'p':
			if ((password = get_password(optarg)) == NULL)
				errx(1, "Invalid SecureOn password: %s", optarg);
			break;
		default:
			errx(1, USAGE, getprogname());
		}
	}

	if (optind >= argc)
		errx(1, USAGE, getprogname());

	if (getifaddrs(&ifaddrs) == -1)
		err(1, "getifaddrs");

	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_port = htons(port);
	sin.sin_addr.s_addr = isdigit((int)ifname[0]) ?
	    get_broadcast_address2(ifname, ifaddrs) :
	    get_broadcast_address(ifname);

	for (i = optind; i < argc; i++) {
		if ((mac = ether_aton(argv[i])) == NULL) {
			if (ether_hostton(argv[i], &ea) != 0)
				errx(1, "Missing in /etc/ethers: %s", argv[i]);
			mac = &ea;
		}

		wake(mac, sin, password);
	}

	freeifaddrs(ifaddrs);

	return (0);
}
