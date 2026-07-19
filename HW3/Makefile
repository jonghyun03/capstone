CC = gcc
CFLAGS = -Wall -Wextra

all: server client

server: hw3_epoll_server.c
	$(CC)	$(CFLAGS) hw3_epoll_server.c -o server

client: hw3_client.c
	$(CC) $(CFLAGS) hw3_client.c -o client

clean:
	rm -f server client

.PHONY: all clean