CC=gcc
SRC:=$(wildcard *.c)
OBJ:=$(SRC:.c=.o)
DFLAGS=-g -Og
RFLAGS=-O2
CFLAGS=-Wall -Wextra -c
OUT=parser_demo

default: CFLAGS += $(RFLAGS)
default: compile
	./parser_demo

debug: CFLAGS += $(DFLAGS)
debug: compile
	gdb parser_demo

compile: $(OBJ)
	$(CC) -o $(OUT) $^

%.o: %.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f *.o parser_demo
