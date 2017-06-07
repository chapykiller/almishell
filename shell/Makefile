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
all: obj/almishell.o
	gcc $(CFLAGS) obj/almishell.o -o main

obj:
	mkdir obj

obj/almishell.o: obj src/almishell.c
	gcc $(CFLAGS) $(INCLUDE) -c src/almishell.c -o obj/almishell.o

clean:
	rm -rf obj/*

install: all
	mkdir -p $(PREFIX)
	mkdir -p $(PREFIX)/include/almishell
	cp -pr include/ $(PREFIX)/include/almishell

uninstall:
	rm -rf $(PREFIX)/include/almishell
