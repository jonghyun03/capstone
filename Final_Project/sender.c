#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <pthread.h>
#include "p2p.h"
#include "sender.h"

typedef struct
{
  int idx;
  int fd;
} sock_idx;

void *sender_send(void *arg);
double get_time_sec(void);

int *each_seg_size;
int *seg_numbers_per_recv; // Segment per Receiver
volatile int complete = 0;

long long *total_bytes_sent;
double transfer_start_time;
pthread_mutex_t mutex;
pthread_mutex_t print_mutex;

void sender(char *argv[])
{
  int i;
  int default_seg_size;
  struct stat mystat;
  int tmp, idx;
  short s_tmp;
  int file_name_len;

  // int serv_sock, clnt_sock;
  int serv_sock;
  struct sockaddr_in serv_adr, clnt_adr;
  socklen_t clnt_adr_sz;
  pthread_t t_id;

  sock_idx *clnt_sock;

  FILE *fp;

  total_seg_number = 0;
  map_count = 0;

  recv_number = atoi(argv[5]);
  default_seg_size = atoi(argv[9]) * 1000;
  seg_numbers_per_recv = (int *)malloc(sizeof(int) * recv_number);
  total_bytes_sent = (long long *)malloc(sizeof(long long));
  *total_bytes_sent = 0;
  pthread_mutex_init(&mutex, NULL);
  pthread_mutex_init(&print_mutex, NULL);

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

  each_seg_size = (int *)malloc(sizeof(int) * total_seg_number);
  for (i = 0; i < total_seg_number; i++)
  {
    each_seg_size[i] = 0;
  }
  tmp = file_size / default_seg_size;
  for (i = 0; i < tmp; i++)
  {
    each_seg_size[i] = default_seg_size;
  }
  tmp = file_size % default_seg_size;
  if (tmp != 0)
  {
    each_seg_size[i] = tmp;
  }

  segment = malloc(total_seg_number * sizeof(char *));
  for (i = 0; i < total_seg_number; i++)
  {
    segment[i] = malloc(default_seg_size * sizeof(char));
    if (segment[i] == NULL)
    {
      for (int j = 0; j < i; j++)
      {
        free(segment[j]);
      }
      free(segment);
      exit(1);
    }
  }

  recv_info.IP = malloc(sizeof(int) * recv_number);
  recv_info.PORT = malloc(sizeof(short) * recv_number);

  serv_sock = socket(PF_INET, SOCK_STREAM, 0);
  int reuse = 1;
  setsockopt(serv_sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

  memset(&serv_adr, 0, sizeof(serv_adr));
  serv_adr.sin_family = AF_INET;
  serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
  serv_adr.sin_port = htons(atoi(argv[2]));

  if (bind(serv_sock, (struct sockaddr *)&serv_adr, sizeof(serv_adr)) == -1)
    error_handling("bind() error");
  if (listen(serv_sock, 10) == -1)
    error_handling("listen() error");

  printf("Listening... \n");

  fp = fopen(file_name, "rb");
  for (i = 0; i < total_seg_number; i++)
  {
    fread((void *)segment[i], 1, each_seg_size[i], fp);
  }

  file_name_len = strlen(file_name);

  int tmp_ac = 0;
  while (1)
  {
    // accept();
    clnt_sock = malloc(sizeof(sock_idx));
    if (clnt_sock == NULL)
      error_handling("malloc() error");
    clnt_adr_sz = sizeof(clnt_adr);
    if (tmp_ac >= recv_number)
      break;
    clnt_sock->fd = accept(serv_sock, (struct sockaddr *)&clnt_adr, &clnt_adr_sz);
    tmp_ac++;
    clnt_sock->idx = map_count;

    // 접속한 receiver의 listen 소켓 포트 번호 받기
    if (read_exact(clnt_sock->fd, &s_tmp, sizeof(s_tmp)) == -1)
      error_handling("read() error");
    /* ======================================================
    접속한 Receiver에게 [총 Receiver 수], [해당 Receiver의 Index], [총 Segement 수], [세그먼트 개당 크기], [파일 크기], [파일 이름 길이], [파일 이름] 전송
    ====================================================== */
    if (write_all(clnt_sock->fd, &recv_number, sizeof(recv_number)) == -1)
      error_handling("write() error");
    if (write_all(clnt_sock->fd, &map_count, sizeof(map_count)) == -1)
      error_handling("write() error");
    if (write_all(clnt_sock->fd, &total_seg_number, sizeof(total_seg_number)) == -1)
      error_handling("write() error");
    if (write_all(clnt_sock->fd, &default_seg_size, sizeof(default_seg_size)) == -1)
      error_handling("write() error");
    if (write_all(clnt_sock->fd, &file_size, sizeof(file_size)) == -1)
      error_handling("write() error");
    if (write_all(clnt_sock->fd, &file_name_len, sizeof(file_name_len)) == -1)
      error_handling("write() error");
    if (write_all(clnt_sock->fd, file_name, file_name_len) == -1)
      error_handling("write() error");
    // map()
    recv_info.PORT[map_count] = ntohs(s_tmp);
    recv_info.IP[map_count] = clnt_adr.sin_addr.s_addr;
    map_count++;

    // printf("Received PORT: %d\n", s_tmp);
    // printf("Received PORT: %d\n", recv_info.PORT[map_count]);

    pthread_create(&t_id, NULL, sender_send, (void *)clnt_sock);
    pthread_detach(t_id);
    // printf("Connected client IP: %s \n", inet_ntoa(clnt_adr.sin_addr));
    // printf("Connected client PORT: %d \n", ntohs(clnt_adr.sin_port));
  }

  system("clear");

  char process[11];
  int proc;
  int bar_count;
  double elapsed_time;
  double mbps;

  transfer_start_time = get_time_sec();

  while (1)
  {
    if (complete != 0)
    {
      long long bytes_sent;

      pthread_mutex_lock(&mutex);
      bytes_sent = *total_bytes_sent;
      pthread_mutex_unlock(&mutex);

      elapsed_time = get_time_sec() - transfer_start_time;
      if (elapsed_time > 0)
        mbps = ((double)bytes_sent * 8.0) / elapsed_time / 1000000.0;
      else
        mbps = 0.0;

      proc = (int)((100 * bytes_sent) / file_size);
      if (proc > 100)
        proc = 100;
      bar_count = proc / 10;

      for (int i = 0; i < 10; i++)
        process[i] = ' ';
      process[10] = '\0';
      for (int i = 0; i < bar_count; i++)
      {
        process[i] = '#';
      }

      pthread_mutex_lock(&print_mutex);
      printf("\033[1;1H");
      printf("\033[K");
      printf("Sending Peer [%s] %d%% (%lld / %lld Bytes) %.1fMbps   (%.1fs)\n", process, proc, bytes_sent, file_size, mbps, elapsed_time);
      fflush(stdout);
      pthread_mutex_unlock(&print_mutex);
      if (bytes_sent >= file_size)
        break;
      usleep(1000);
    }
    else
    {
      usleep(1000);
    }
  }
  printf("\n\n");
}

// 리시버들에게 모든 리시버 정보 전달
void *sender_send(void *arg)
{
  int i;
  int ptr = 0;
  long long my_bytes_sent = 0;
  double peer_start_time;
  double elapsed_time;
  double mbps;
  sock_idx sock = *((sock_idx *)arg);
  free(arg);
  pkt pkt_s;
  // int sock = *((int *)arg);
  int complete_signal;

  while (1)
  {
    if (recv_number == map_count)
    {
      for (int i = 0; i < recv_number; i++)
      {
        /* ======================================================
        Send All_Receiver_Info to current Receiver
        ====================================================== */
        if (write_all(sock.fd, &recv_info.IP[i], sizeof(recv_info.IP[i])) == -1)
          error_handling("write() error");
        if (write_all(sock.fd, &recv_info.PORT[i], sizeof(recv_info.PORT[i])) == -1)
          error_handling("write() error");
      }

      break;
    }
    usleep(5000000);
    // sleep(1);
  }
  // 다 연결 되었는지 확인을 받음
  if (read_exact(sock.fd, &complete_signal, sizeof(complete_signal)) == -1)
    error_handling("read() error");
  complete = complete_signal;
  peer_start_time = get_time_sec();

  // 보낼 segment 갯수 보내기;
  if (write_all(sock.fd, &seg_numbers_per_recv[sock.idx], sizeof(seg_numbers_per_recv[sock.idx])) == -1)
    error_handling("write() error");

  for (i = 0; i < total_seg_number; i++)
  {
    if (i % recv_number == sock.idx)
    {
      // Segment IDX
      if (write_all(sock.fd, &i, sizeof(i)) == -1)
        error_handling("write() error");
      // TOTAL SIZE
      if (write_all(sock.fd, &each_seg_size[i], sizeof(each_seg_size[i])) == -1)
        error_handling("write() error");
      ptr = 0;
      while (1)
      {

        if (each_seg_size[i] - ptr > 1000)
        {
          pkt_s.size = 1000;
          memcpy(pkt_s.data, segment[i] + ptr, 1000);
          ptr += 1000;
        }
        else
        {
          pkt_s.size = each_seg_size[i] - ptr;
          memcpy(pkt_s.data, segment[i] + ptr, pkt_s.size);
          ptr += pkt_s.size;
        }
        if (write_all(sock.fd, &pkt_s.size, sizeof(pkt_s.size)) == -1)
          error_handling("write() error");
        if (write_all(sock.fd, pkt_s.data, pkt_s.size) == -1)
          error_handling("write() error");
        my_bytes_sent += pkt_s.size;
        pthread_mutex_lock(&mutex);
        *total_bytes_sent += pkt_s.size;
        pthread_mutex_unlock(&mutex);

        elapsed_time = get_time_sec() - peer_start_time;
        if (elapsed_time > 0)
          mbps = ((double)my_bytes_sent * 8.0) / elapsed_time / 1000000.0;
        else
          mbps = 0.0;

        pthread_mutex_lock(&print_mutex);
        printf("\033[%d;1H", sock.idx + 2);
        printf("\033[K");
        printf("To Receiving Peer #%d: %.1f Mbps (%lld Bytes Sent, %.1fs)\n", sock.idx + 1, mbps, my_bytes_sent, elapsed_time);
        fflush(stdout);
        pthread_mutex_unlock(&print_mutex);
        if (ptr >= each_seg_size[i])
          break;
      }
    }
    //    write(fd, segment->size, int)
    //    while(1){send data}
  }

  return NULL;
}

double get_time_sec(void)
{
  struct timespec ts;

  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (double)ts.tv_sec + ((double)ts.tv_nsec / 1000000000.0);
}