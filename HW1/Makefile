CC = gcc
CFLAGS = -Wall -Wextra

all: server client

server: HW1_FINAL_S.c
	$(CC)	$(CFLAGS) HW1_FINAL_S.c -o server

client: HW1_FINAL_C.c
	$(CC) $(CFLAGS) HW1_FINAL_C.c -o client

clean:
	rm -f server client

.PHONY: all clean