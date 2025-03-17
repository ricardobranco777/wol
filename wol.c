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

#define USAGE "%s [-i interface] [-P port] [-p password] lladdr..."

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

	if (ifname == NULL || ifname[0] == '\0')
		return INADDR_BROADCAST;

	if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
		return INADDR_BROADCAST;

	memset(&ifr, 0, sizeof(ifr));
	strlcpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name));

	if (ioctl(sock, SIOCGIFBRDADDR, &ifr) == -1) {
		(void)close(sock);
		return INADDR_BROADCAST;
	}

	(void)close(sock);
	return ((struct sockaddr_in *)&ifr.ifr_broadaddr)->sin_addr.s_addr;
}

static uint8_t *
get_password(const char *str)
{
	static uint8_t password[6];
	unsigned int values[6];
	int i;

	if (sscanf(str, "%2x:%2x:%2x:%2x:%2x:%2x",
	   &values[0], &values[1], &values[2],
	   &values[3], &values[4], &values[5]) != 6)
		return NULL;

	for (i = 0; i < 6; i++)
		password[i] = (uint8_t)values[i];

	return password;
}

static void
wake_on_lan(const char *target, struct sockaddr_in sin, const uint8_t *password) {
	uint8_t payload[102 + 6];
	struct ether_addr *mac;
	size_t size;
	int on = 1;
	int sock;

	if ((mac = resolve_mac(target)) == NULL)
		errx(1, "Invalid MAC address or hostname: %s", target);

	memset(payload, 0xFF, 6);
	for (size = 6; size < sizeof(payload); size += 6)
		memcpy(payload + size, mac->ether_addr_octet, 6);

	if (password != NULL) {
		memcpy(payload + 102, password, 6);
		size += 6;
	}

	if ((sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
		err(1, "socket");

	if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &on, sizeof(int)) == -1)
		err(1, "setsockopt");

	printf("Sending to %s via %s\n", ether_ntoa(mac), inet_ntoa(sin.sin_addr));

	if (sendto(sock, payload, size, 0, (struct sockaddr*)&sin, sizeof(sin)) == -1)
		err(1, "sendto");

	(void)close(sock);
}

int
main(int argc, char *argv[]) {
	char ifname[IF_NAMESIZE] = {0};
	uint8_t *password = NULL;
	struct sockaddr_in sin;
	uint16_t port = 9;
	int i, opt;

	while ((opt = getopt(argc, argv, "i:P:p:")) != -1) {
		switch (opt) {
		case 'i':
			strlcpy(ifname, optarg, sizeof(ifname));
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

	memset(&sin, 0, sizeof(sin));
	sin.sin_addr.s_addr = get_broadcast_address(ifname);
	sin.sin_family = AF_INET;
	sin.sin_port = htons(port);

	for (i = optind; i < argc; i++)
		wake_on_lan(argv[i], sin, password);

	return (0);
}
