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
#include <sys/ethernet.h>
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

static struct ether_addr *
resolve_mac(const char *name)
{
	static struct ether_addr ea;

	if (ether_hostton(name, &ea) == 0)
		return &ea;

	return ether_aton(name);
}

static void
wake_on_lan(const char *target) {
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
	sa.sin_addr.s_addr = INADDR_BROADCAST;
	sa.sin_family = AF_INET;
	sa.sin_port = htons(9);

	if ((sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
		err(1, "socket");

	if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &on, sizeof(int)) == -1)
		err(1, "setsockopt");

	printf("Sending to %s\n", ether_ntoa(mac));

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
