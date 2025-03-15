/*
 * According to Richard W. Stevens's UNIX Network Programming Vol. 1 3rd Ed:
 *
 * "Calling connect on a UDP socket does not send anything to that host;
 *  it is entirely a local operation that saves the peer's IP address and port.
 *  We also see that calling connect on an unbound UDP socket also assigns an
 *  ephemeral port to the socket.
 *  Unfortunately, this technique does not work on all implementations, mostly
 *  SVR4-derived kernels. For example, this does not work on Solaris 2.5, but it
 *  works on AIX, HP-UX 11, MacOS X, FreeBSD, Linux, and Solaris 2.6 and later."
 *
 * Verified to work on Linux, FreeBSD, NetBSD, OpenBSD, DragonflyBSD & Illumos
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <err.h>
#include <errno.h>
#include <getopt.h>
#include <unistd.h>

#define USAGE	"Usage: %s [-4|-6] TARGET"

#define PORT	60000

union sockaddr_union {
	struct sockaddr_in v4;
	struct sockaddr_in6 v6;
};

int
main(int argc, char *argv[])
{
	union sockaddr_union source, remote;
	struct addrinfo hints, *res, *ai;
	char source_addr[INET6_ADDRSTRLEN];
	char target_addr[INET6_ADDRSTRLEN];
	int family = AF_UNSPEC;
	int opt, error, sock;
	socklen_t namelen;
	char *target;

	while ((opt = getopt(argc, argv, "46")) != -1) {
		switch (opt) {
		case '4':
			family = AF_INET;
			break;
		case '6':
			family = AF_INET6;
			break;
		default:
			errx(1, USAGE, argv[0]);
		}
	}

	if (optind >= argc)
		errx(1, USAGE, argv[0]);
	target = argv[optind];

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = family;
	hints.ai_socktype = SOCK_DGRAM;

	if ((error = getaddrinfo(target, NULL, &hints, &res)) != 0)
		errx(1, "getaddrinfo: %s: %s", target, gai_strerror(error));

	for (ai = res; ai != NULL; ai = ai->ai_next) {
		memset(&remote, 0, sizeof(remote));
		memcpy(&remote, ai->ai_addr, ai->ai_addrlen);
		if (ai->ai_family == AF_INET)
			remote.v4.sin_port = htons(PORT);
		else if (ai->ai_family == AF_INET6)
			remote.v6.sin6_port = htons(PORT);
		else
			continue;

		if ((sock = socket(ai->ai_family, SOCK_DGRAM, 0)) == -1)
			err(1, "socket");
		if (connect(sock, (struct sockaddr *)&remote, ai->ai_addrlen) == -1) {
			if (errno == EHOSTUNREACH || errno == ENETUNREACH) {
				(void)close(sock);
				continue;
			} else
				err(1, "connect");
		}

		namelen = ai->ai_addrlen;
		if (getsockname(sock, (struct sockaddr *)&source, &namelen) == -1)
			err(1, "getsockname");
		(void)close(sock);

		if (inet_ntop(ai->ai_family, ai->ai_family == AF_INET
		    ? (void *)&remote.v4.sin_addr
		    : (void *)&remote.v6.sin6_addr,
		    target_addr, sizeof(target_addr)) == NULL)
			err(1, "inet_ntop");

		if (inet_ntop(ai->ai_family, ai->ai_family == AF_INET
		    ? (void *)&source.v4.sin_addr
		    : (void *)&source.v6.sin6_addr,
		    source_addr, sizeof(source_addr)) == NULL)
			err(1, "inet_ntop");

		printf("%s (%s) via %s\n", target, target_addr, source_addr);
	}

	freeaddrinfo(res);
	return (0);
}
