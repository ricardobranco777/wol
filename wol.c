#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <ctype.h>
#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_TRIES 10

static int
xdigit (char c)
{
	if (c >= '0' && c <= '9')
		return (c - '0');
	c |= 0x20; // c = tolower(c);
	if (c >= 'a' && c <= 'f')
		return (10 + c - 'a');
	return (-1);
}

static uint8_t *
ether_str2addr(const char *str, uint8_t *addr)
{
	int v, x;

	for (int i = 0; i < 6; i++) {
		if ((x = xdigit(*str++)) < 0)
			return NULL;
		v = x << 4;
		if ((x = xdigit(*str++)) < 0)
			return NULL;
		v |= x;
		addr[i] = v;
		if (i < 5 && !isxdigit((int)*str))
			str++;
	}
	if (*str != '\0')
		return NULL;

	return addr;
}

static void
wake_on_lan(const char *mac_address) {
	uint8_t payload[102];
	uint8_t mac[6];

	if (ether_str2addr(mac_address, mac) == NULL)
		errx(1, "Invalid MAC address: %s", mac_address);

	memset(payload, 0xFF, 6);
	for (unsigned int i = 6; i < sizeof(payload); i += 6)
		memcpy(payload + i, mac, 6);

	struct sockaddr_in sa;
	memset(&sa, 0, sizeof(sa));
	sa.sin_family = AF_INET;
	sa.sin_port = htons(9);
	inet_pton(AF_INET, "255.255.255.255", &sa.sin_addr);

	int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sock < 0)
		err(1, "%s", "socket");

	int on = 1;
	if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &on, sizeof(int)) < 0)
		err(1, "%s", "setsockopt");

	printf("Sending to %02x:%02x:%02x:%02x:%02x:%02x\n",
		mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

	for (int i = 0; i < MAX_TRIES; i++)
		if (sendto(sock, payload, sizeof(payload), 0, (struct sockaddr*)&sa, sizeof(sa)) < 0)
			err(1, "sendto");

	close(sock);
}

int
main(int argc, char *argv[]) {
	if (argc < 2) {
		fprintf(stderr, "Usage: %s MAC_ADDRESS...\n", argv[0]);
		exit(1);
	}

	for (int i = 1; i < argc; i++)
		wake_on_lan(argv[i]);

	return (0);
}
