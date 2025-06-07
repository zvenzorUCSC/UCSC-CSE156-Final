# Makefile

CC = gcc
CFLAGS = -Wall -Wextra -pthread
SRC = src/myproxy.c
BIN = bin/myproxy

all: $(BIN)

$(BIN): $(SRC)
	@mkdir -p bin
	$(CC) $(CFLAGS) -o $(BIN) $(SRC)

clean:
	rm -rf bin/*

.PHONY: all clean
