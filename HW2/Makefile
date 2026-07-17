CC = gcc
CFLAGS = -Wall -Wextra

all: sender receiver

sender: HW2_sender.c
	$(CC)	$(CFLAGS) HW1_FINAL_S.c -o sender

receiver: HW2_receiver.c
	$(CC) $(CFLAGS) HW2_receiver.c -o receiver

clean:
	rm -f sender receiver

.PHONY: all clean