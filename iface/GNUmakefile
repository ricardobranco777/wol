BIN	= iface
CC	?= gcc
CFLAGS	= -Wall -Wextra -O2
LDFLAGS	=

OSTYPE	!= uname
ifeq ($(OSTYPE),SunOS)
LDFLAGS	+= -lsocket
endif

$(BIN):	$(BIN).c
	$(CC) -o $@ $(BIN).c $(LDFLAGS) $(CFLAGS)

.PHONY: clean
clean:
	$(RM) $(BIN)
