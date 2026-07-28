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
} segment;

int argument_check(char *argv[], int argc);
int type_check(char str[]);
void error_handling(char *msg);

void sender(char *argv[]);
void *sender_send(void *arg);

int map_count;
int recv_number;
int total_seg_number;
long long file_size;
char file_name[64];



receiver_info recv_info;

int main(int argc, char *argv[])
{
  int issender;
  // sender - receiver check
  if (argument_check(argv, argc))
    exit(1);

  issender = type_check(argv[1]);

  memset(file_name, 0, sizeof(file_name));

  if (issender)
    sender(argv);

  // receiver_info *recv_info = malloc(sizeof(receiver_info));
  // recv_info->IP = malloc(sizeof(int)*recv_count);
  // recv_info->PORT = malloc(sizeof(int)*recv_count);

  return 0;
}

void sender(char *argv[])
{
  int i;
  int default_seg_size;
  int *seg_numbers_per_recv; // Segment per Receiver
  struct stat mystat;
  int tmp, idx;
  short s_tmp;

  int serv_sock, clnt_sock;
  struct sockaddr_in serv_adr, clnt_adr;
  int clnt_adr_sz;
  pthread_t t_id;

  total_seg_number = 0;
  map_count = 0;

  recv_number = atoi(argv[5]);
  default_seg_size = atoi(argv[9]) * 1000;
  seg_numbers_per_recv = (int *)malloc(sizeof(int) * recv_number);

  for (i = 0; i < recv_number; i++)
    seg_numbers_per_recv[i] = 0;

  strcpy(file_name, argv[7]);

  if (stat(file_name, &mystat) == -1)
  {
    printf("%s[Error]%s -- Wrong File Requested\n", COLOR_ORANGE, COLOR_RESET);
    printf("FILE NAME: %s\n", file_name);
    exit(1);
  }
  file_size = (long long)mystat.st_size;

  printf("FILE SIZE: %lld, SEG SIZE: %d\n", file_size, default_seg_size);

  // 각 리시버 별로 Segment 갯수 할당
  tmp = file_size / default_seg_size;
  for (i = 0; i < tmp; i++)
  {
    idx = i % recv_number;
    seg_numbers_per_recv[idx]++;
    total_seg_number++;
  }
  tmp = file_size % default_seg_size;
  if (tmp != 0)
  {
    total_seg_number++;
    idx = i % recv_number;
    seg_numbers_per_recv[idx]++;
  }

  // DEGUG
  for (i = 0; i < recv_number; i++)
  {
    printf("%d# Seg Count: %d\n", i, seg_numbers_per_recv[i]);
  }

  recv_info.IP = malloc(sizeof(int) * recv_number);
  recv_info.PORT = malloc(sizeof(int) * recv_number);

  serv_sock = socket(PF_INET, SOCK_STREAM, 0);

  memset(&serv_adr, 0, sizeof(serv_adr));
  serv_adr.sin_family = AF_INET;
  serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
  serv_adr.sin_port = htons(atoi(argv[2]));

  if (bind(serv_sock, (struct sockaddr *)&serv_adr, sizeof(serv_adr)) == -1)
    error_handling("bind() error");
  if (listen(serv_sock, 5) == -1)
    error_handling("listen() error");

  printf("Listening... \n");
  
  while (1)
  {
    // accept();
    clnt_adr_sz = sizeof(clnt_adr);
    clnt_sock = accept(serv_sock, (struct sockaddr *)&clnt_adr, &clnt_adr_sz);

    // 접속한 receiver의 listen 소켓 포트 번호 받기
    read(clnt_sock, (short int*)&s_tmp, sizeof(short));
    // 접속한 receiver에게 해당 receiver의 Index 전송
    write(clnt_sock, (int*)&map_count, sizeof(map_count));
    printf("Received PORT: %d\n", s_tmp);
    recv_info.PORT[map_count] = ntohs(s_tmp);
    printf("Received PORT: %d\n", recv_info.PORT[map_count]);

    // map()
    map_count++;

    pthread_create(&t_id, NULL, sender_send, (void *)&clnt_sock);
    pthread_detach(t_id);
    printf("Connected client IP: %s \n", inet_ntoa(clnt_adr.sin_addr));
    printf("Connected client PORT: %d \n", ntohs(clnt_adr.sin_port));

    // pthread_create(sender_send());
    // pthread_detach();
  }
}

// 리시버들에게 모든 리시버 정보 전달
void *sender_send(void *arg)
{
  int sock = *((int*)arg);
  write(sock, (int*)&recv_number, sizeof(recv_number));
  while (1)
  {
    if (recv_number == map_count)
    {
      for(int i=0; i<recv_number; i++)
        printf("%d\n", recv_info.PORT[i]);
      break;
      // write(fd, recv_number, sizeof(int));
      // write(fd, idx, sizeof(int));
      // write(fd, recv_info->IP, sizeof(int)*recv_number);
      // write(fd, recv_info->PORT, sizeof(int)*recv_number);
      // break;
    }
    else break;
  }
  // 다 연결 되었는지 확인을 받음
  // read(fd, confirm, 1);

  // 보낼 segment 갯수 보내기;
  // write(fd, seg_count, 4);
  // for(i=0; i<segcount; i++){
  //    write(fd, segment->size, int)
  //    while(1){send data}
  // }
}

int type_check(char str[])
{
  if (strcpy(str, "-s"))
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
