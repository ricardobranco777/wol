#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <net/if.h>
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
#include <err.h>
#include <getopt.h>

#ifdef __linux__
extern char *__progname;
#define getprogname()   (__progname)
#endif

#define USAGE "%s [-i interface] lladdr..."

static struct ether_addr *
resolve_mac(const char *name)
{
	static struct ether_addr ea;

	if (ether_hostton(name, &ea) == 0)
		return &ea;

	return ether_aton(name);
}

static in_addr_t
get_broadcast_address(const char *ifname)
{
	struct ifreq ifr;
	int sock;

	if (ifname == NULL)
		return INADDR_BROADCAST;

	if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
		return INADDR_BROADCAST;

	memset(&ifr, 0, sizeof(ifr));
	strncpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name) - 1);

	if (ioctl(sock, SIOCGIFBRDADDR, &ifr) == -1) {
		(void)close(sock);
		return INADDR_BROADCAST;
	}

	(void)close(sock);
	return ((struct sockaddr_in *)&ifr.ifr_broadaddr)->sin_addr.s_addr;
}

static void
wake_on_lan(const char *target, const char *ifname) {
	struct ether_addr *mac;
	struct sockaddr_in sa;
	uint8_t payload[102];
	unsigned int i;
	int on = 1;
	int sock;

	if ((mac = resolve_mac(target)) == NULL)
		errx(1, "Invalid MAC address or hostname: %s", target);

	memset(payload, 0xFF, 6);
	for (i = 6; i < sizeof(payload); i += 6)
		memcpy(payload + i, mac->ether_addr_octet, 6);

	memset(&sa, 0, sizeof(sa));
	sa.sin_addr.s_addr = get_broadcast_address(ifname);
	sa.sin_family = AF_INET;
	sa.sin_port = htons(9);

	if ((sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
		err(1, "socket");

	if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &on, sizeof(int)) == -1)
		err(1, "setsockopt");

	printf("Sending to %s via %s\n", ether_ntoa(mac), inet_ntoa(sa.sin_addr));

	if (sendto(sock, payload, sizeof(payload), 0, (struct sockaddr*)&sa, sizeof(sa)) == -1)
		err(1, "sendto");

	(void)close(sock);
}

int
main(int argc, char *argv[]) {
	const char *ifname = NULL;
	int i, opt;

	while ((opt = getopt(argc, argv, "i:")) != -1) {
		switch (opt) {
		case 'i':
			ifname = optarg;
			break;
		default:
			errx(1, USAGE, getprogname());
		}
	}

	if (optind >= argc)
		errx(1, USAGE, getprogname());

	for (i = optind; i < argc; i++)
		wake_on_lan(argv[i], ifname);

	return (0);
}
