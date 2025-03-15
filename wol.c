#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#if defined(__linux__)
#include <netinet/ether.h>
#elif defined(__FreeBSD__) || defined(__DragonFly__)
#include <net/ethernet.h>
#define	ether_addr_octet octet
#elif defined(__sun__)
#include <net/ethernet.h>
#elif defined(__NetBSD__)
#include <net/if.h>
#include <net/if_ether.h>
#elif defined(__OpenBSD__)
#include <net/if.h>
#include <netinet/if_ether.h>
#else
#error not supported
#endif

#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef __linux__
extern char *__progname;
#define getprogname()   (__progname)
#endif

static int
resolve_mac(const char *input, uint8_t *mac)
{
	struct ether_addr *eth;

	struct ether_addr eth_resolved;
	if (ether_hostton(input, &eth_resolved) == 0) {
		memcpy(mac, eth_resolved.ether_addr_octet, 6);
		return 0;
	}

	if ((eth = ether_aton(input)) != NULL) {
		memcpy(mac, eth->ether_addr_octet, 6);
		return 0;
	}

	return -1;
}

static void
wake_on_lan(const char *target) {
	struct sockaddr_in sa;
	uint8_t payload[102];
	uint8_t mac[6];
	unsigned int i;
	int on = 1;
	int sock;

	if (resolve_mac(target, mac) == -1)
		errx(1, "Invalid MAC address or hostname: %s", target);

	memset(payload, 0xFF, 6);
	for (i = 6; i < sizeof(payload); i += 6)
		memcpy(payload + i, mac, 6);

	memset(&sa, 0, sizeof(sa));
	sa.sin_family = AF_INET;
	sa.sin_port = htons(9);
	inet_pton(AF_INET, "255.255.255.255", &sa.sin_addr);

	if ((sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
		err(1, "socket");

	if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &on, sizeof(int)) == -1)
		err(1, "setsockopt");

	printf("Sending to %02x:%02x:%02x:%02x:%02x:%02x\n",
		mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

	if (sendto(sock, payload, sizeof(payload), 0, (struct sockaddr*)&sa, sizeof(sa)) == -1)
		err(1, "sendto");

	(void)close(sock);
}

int
main(int argc, char *argv[]) {
	if (argc < 2)
		errx(1, "Usage: %s lladdr...", getprogname());

	for (int i = 1; i < argc; i++)
		wake_on_lan(argv[i]);

	return (0);
}
