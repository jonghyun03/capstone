#ifndef _P2P_H
#define _P2P_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pthread.h>

#define COLOR_ORANGE "\033[38;2;255;140;0m"
#define COLOR_RESET "\033[0m"

typedef struct
{
  int *IP;
  short *PORT;
} receiver_info;

typedef struct
{
  int time;
  int size;
  char data[1000];
} pkt;

void error_handling(char *msg);
int read_exact(int fd, void *buf, size_t len);
int write_all(int fd, const void *buf, size_t len);

extern int map_count;
extern int recv_number;
extern int total_seg_number;
extern long long file_size;
extern char file_name[64];
extern char **segment;
extern receiver_info recv_info;

#endif