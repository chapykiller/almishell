CFLAGS = -Wall -Werror --ansi --pedantic-errors -D_POSIX_C_SOURCE=200809L
LDFLAGS = -shared
INCLUDE = -Iinclude
CC = gcc

ifndef PREFIX
PREFIX = /usr/local
endif

.PHONY: all clean install uninstall
all: lib/libruncmd.so

lib/libruncmd.so: obj/runcmd.o
	$(CC) $(LDFLAGS) -o libruncmd.so obj/runcmd.o

obj/runcmd.o: src/runcmd.c
	$(CC) $(CFLAGS) -fPIC $(INCLUDE) -c src/runcmd.c -o obj/runcmd.o

clean:
	rm -rf obj/* lib/*

install:
	install $(PREFIX)/lib ./lib/libruncmd.so
	install $(PREFIX)/include ./include/runcmd.h

uninstall: $(PREFIX)/lib/libruncmd.so
	rm -rf $^
