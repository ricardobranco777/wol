OPSYS:sh = uname
PROG=	iface
WARNS=	3
CFLAGS+= -O2

MK_DEBUG_FILES=	no
MAN=

.if ${OPSYS} == "OpenBSD"
CFLAGS+= -Wall -Wextra
.endif

.if ${OPSYS} == "NetBSD"
BINDIR= /usr/pkg/bin
.else
BINDIR= /usr/local/bin
.endif

.include <bsd.prog.mk>
