CC=gcc
CFLAGS=-Wall -g
BIN=bin/main
OBJS=obj/main.o obj/LCD.o obj/GC9A01A.o obj/assets.o

all:$(BIN)

bin/main: $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -I ./src -o $@

obj/%.o: src/%.c 
	$(CC) $(CFLAGS) -c $< -o $@







