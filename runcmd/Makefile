CFLAGS = -Wall -Werror --ansi --pedantic-errors -D_POSIX_C_SOURCE=200809L
LDFLAGS = -shared
INCLUDE = -Iinclude/runcmd
CC = gcc

ifndef PREFIX
PREFIX = /usr/local
endif

.PHONY: all clean install uninstall
all: lib/libruncmd.so lib/libruncmd.a

lib:
	mkdir lib

obj:
	mkdir obj

lib/libruncmd.so: lib obj/runcmd.o
	$(CC) $(LDFLAGS) -o lib/libruncmd.so obj/runcmd.o

lib/libruncmd.a: lib obj/runcmd.o
	ar -cvq lib/libruncmd.a obj/runcmd.o

obj/runcmd.o: obj src/runcmd.c
	$(CC) $(CFLAGS) -fPIC $(INCLUDE) -c src/runcmd.c -o obj/runcmd.o

clean:
	rm -rf obj/* lib/*

install:
	install ./lib/libruncmd.so $(PREFIX)/lib
	install ./lib/libruncmd.a $(PREFIX)/lib
	mkdir -p $(PREFIX)/include/runcmd
	install ./include/runcmd/runcmd.h $(PREFIX)/include/runcmd

uninstall: $(PREFIX)/lib/libruncmd.so $(PREFIX)/lib/libruncmd.a
	rm -f $^
	rm -rf $(PREFIX)/include/runcmd
