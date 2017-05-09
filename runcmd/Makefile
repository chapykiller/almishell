CFLAGS = -Wall -Werror --ansi --pedantic-errors -D_POSIX_C_SOURCE=200809L
CC = gcc

ifndef PREFIX
PREFIX = /usr/local
endif

all: obj/runcmd.o
	$(CC) $(CFLAGS) obj/runcmd.o -o bin/runcmd

runcmd.o: src/runcmd.c
	$(CC) $(CFLAGS) -c src/runcmd.c

clean:
	rm -rf obj/* bin/*

install:
	install -D $(PREFIX) ./{bin,include}

uninstall: $(PREFIX)/bin/runcmd $(PREFIX)/include/runcmd
	rm -rf $(PREFIX)/bin/runcmd $(PREFIX)/include/runcmd
