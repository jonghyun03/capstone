#include "p2p.h"
#include "sender.h"
#include "receiver.h"

// typedef struct
// {
//   int *IP;
//   short *PORT;
// } receiver_info;

// typedef struct
// {
//   int time;
//   int size;
//   char data[1000];
// } pkt;

int argument_check(char *argv[], int argc);
int type_check(char str[]);


int map_count;
int recv_number;
int total_seg_number;
long long file_size;
char file_name[64];
char **segment;



receiver_info recv_info;

int main(int argc, char *argv[])
{
  int issender;
  // sender - receiver check
  if (argument_check(argv, argc))
    exit(1);

  issender = type_check(argv[3]);

  memset(file_name, 0, sizeof(file_name));

  if (issender)
    sender(argv);
  else
    receiver(argv);

  // receiver_info *recv_info = malloc(sizeof(receiver_info));
  // recv_info->IP = malloc(sizeof(int)*recv_count);
  // recv_info->PORT = malloc(sizeof(int)*recv_count);

  return 0;
}

int type_check(char str[])
{
  if (strcmp(str, "-s") == 0)
    return 1;
  else
    return 0;
}

int argument_check(char *argv[], int argc)
{
  if ((strcmp(argv[3], "-r") != 0) && (strcmp(argv[3], "-s") != 0))
  {
    printf("CASE 1\n");
    printf("Usage : %s -p <LISTEN_PORT> -s -n <segment_number> -f <file_name> -g <segment_size>\n", argv[0]);
    printf("OR\n");
    printf("Usage : %s -p <LISTEN_PORT< -r -a <IP> <CONNECT_PORT>\n", argv[0]);
    return 1;
  }
  if ((strcmp(argv[3], "-s") == 0) && (argc != 10))
  {
    printf("CASE 2\n");
    printf("Usage : %s -p <LISTEN_PORT> -s -n <segment_number> -f <file_name> -g <segment_size>\n", argv[0]);
    return 1;
  }
  if ((strcmp(argv[3], "-r") == 0) && (argc != 7))
  {
    printf("CASE 3\n");
    printf("Usage : %s -p <LISTEN_PORT< -r -a <IP> <CONNECT_PORT>\n", argv[0]);
    return 1;
  }
  if ((strcmp(argv[1], "-p") == 0) && (strcmp(argv[3], "-s") == 0) && (strcmp(argv[4], "-n") != 0) && (atoi(argv[5]) == 0) && (strcmp(argv[6], "-f") == 0) && (strcmp(argv[8], "-g") == 0) && (atoi(argv[9]) == 0))
  {
    printf("CASE 4\n");
    printf("Usage : %s -p <LISTEN_PORT> -s -n <segment_number> -f <file_name> -g <segment_size>\n", argv[0]);
    return 1;
  }
  if ((strcmp(argv[1], "-p") == 0) && (strcmp(argv[3], "-r") == 0) && (strcmp(argv[4], "-a") != 0))
  {
    printf("CASE 5\n");
    printf("Usage : %s -p <LISTEN_PORT< -r -a <IP> <CONNECT_PORT>\n", argv[0]);
    return 1;
  }

  return 0;
}

void error_handling(char *msg)
{
  fputs(msg, stderr);
  fputc('\n', stderr);
  exit(1);
}

int read_exact(int fd, void *buf, size_t len)
{
  size_t total = 0;
  char *ptr = (char *)buf;

  while (total < len)
  {
    ssize_t n = read(fd, ptr + total, len - total);
    if (n <= 0)
      return -1;
    total += n;
  }

  return 0;
}

int write_all(int fd, const void *buf, size_t len)
{
  size_t total = 0;
  const char *ptr = (const char *)buf;

  while (total < len)
  {
    ssize_t n = write(fd, ptr + total, len - total);
    if (n <= 0)
      return -1;
    total += n;
  }

  return 0;
}