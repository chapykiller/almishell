CFLAGS = -Wall -Werror --ansi --pedantic-errors -D_POSIX_C_SOURCE=200809L
INCLUDE = -Iinclude
CC = gcc

CL = $(wildcard src/*.c) #cpp list
HL = $(wildcard include/*.h)    #header list
OL = $(patsubst src/%.c, obj/%.o, $(CL) ) #object

RESULTS_DIR = bin obj

ifndef PREFIX
PREFIX = /usr/local
endif

ifdef DEBUG
CFLAGS += -g -fsanitize=undefined
endif

.PHONY: all clean install uninstall
all: bin $(OL)
	gcc $(CFLAGS) $(OL) -o bin/main

$(RESULTS_DIR):
	mkdir -p $@

obj/%.o: src/%.c obj; $(CC) $(CFLAGS) $(INCLUDE) -c $< -o $@

obj/almishell.o: obj src/almishell.c
	gcc $(CFLAGS) $(INCLUDE) -c src/almishell.c -o obj/almishell.o

clean:
	rm -rf $(RESULTS_DIR)

install: all
	mkdir -p $(PREFIX)/include/almishell
	cp -pr include/* $(PREFIX)/include/almishell
	cp -p bin/main $(PREFIX)/bin

uninstall:
	rm -rf $(PREFIX)/include/almishell $(PREFIX)/bin
