OPSYS:sh = uname
PROG=	wol
WARNS=	3
WARNINGS= yes
CFLAGS+= -O2

MK_DEBUG_FILES=	no
MAN=

.if ${OPSYS} == "NetBSD"
BINDIR= /usr/pkg/bin
.else
BINDIR= /usr/local/bin
.endif

.include <bsd.prog.mk>
