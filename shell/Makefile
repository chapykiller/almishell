CFLAGS = -Wall -Werror --ansi --pedantic-errors -D_POSIX_C_SOURCE=200809L
INCLUDE = -Iinclude
CC = gcc

ifndef PREFIX
PREFIX = /usr/local
endif

ifdef DEBUG
CFLAGS += -g
endif

.PHONY: all clean install uninstall
all: bin obj/almishell.o
	gcc $(CFLAGS) obj/almishell.o -o bin/main

bin:
	mkdir bin

obj:
	mkdir obj

obj/almishell.o: obj src/almishell.c
	gcc $(CFLAGS) $(INCLUDE) -c src/almishell.c -o obj/almishell.o

clean:
	rm -rf obj/* bin/*

install: all
	mkdir -p $(PREFIX)/include/almishell
	# cp -pr include/ $(PREFIX)/include/almishell
	cp -p bin/main $(PREFIX)/bin

uninstall:
	rm -rf $(PREFIX)/include/almishell $(PREFIX)/bin
