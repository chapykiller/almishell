CFLAGS = -Wall -Werror --ansi --pedantic-errors -D_POSIX_C_SOURCE=200809L
LDFLAGS = -shared
INCLUDE = -Iinclude
CC = gcc

ifndef PREFIX
PREFIX = /usr/local
endif

.PHONY: all clean install uninstall
all: lib obj/runcmd.lo
	mkdir -p $(PREFIX)
	mkdir -p $(PREFIX)/include
	mkdir -p $(PREFIX)/lib
	libtool --mode=link $(CC) $(CFLAGS) -o lib/libruncmd.la obj/runcmd.lo -rpath $(PREFIX)/lib

lib:
	mkdir lib

obj:
	mkdir obj

obj/runcmd.lo: obj src/runcmd.c
	libtool --mode=compile $(CC) $(CFLAGS) $(INCLUDE) -no-suppress -c src/runcmd.c -o obj/runcmd.lo

clean:
	rm -rf lib/.libs/
	rm -rf obj/* lib/*

install: all
	libtool --mode=install install lib/libruncmd.la $(PREFIX)/lib
	mkdir -p $(PREFIX)/include/runcmd
	cp include/runcmd/runcmd.h $(PREFIX)/include/runcmd/runcmd.h

uninstall:
	libtool --mode=uninstall rm -f $(PREFIX)/lib/libruncmd.la
	rm -rf $(PREFIX)/include/runcmd
