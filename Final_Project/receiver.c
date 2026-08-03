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
#include "receiver.h"

int my_idx;
int my_seg_count; // 나에게 배정된 Segment 갯수
int *each_seg_size_r;

volatile int recv_seg_count;
volatile int sender_receive_done;
volatile int send_complete_count;

long long total_bytes_recv;
long long sender_bytes_recv;
long long *peer_bytes_recv;
double receiver_start_time;
double sender_start_time;
double *peer_start_time;
pthread_mutex_t recv_mutex;
pthread_mutex_t recv_print_mutex;

double receiver_get_time_sec(void);
void receiver_add_sender_bytes(int size);
void receiver_add_peer_bytes(int peer_idx, int size);
void receiver_print_status(void);

void receiver(char *argv[])
{
  int serv_sock, clnt_sock, sender_sock;
  short my_listen_port;
  struct sockaddr_in serv_adr, clnt_adr, sender_adr;
  struct sockaddr_in peer_adr;

  FILE *fp;

  pthread_t send_thread, recv_thread;

  int default_seg_size;
  int file_name_len;
  int recv_info_flag = 0;
  int seg_no;
  int seg_size;
  // int recv_size = 0;
  // int recv1, recv2;
  int total_recv = 0;
  // int clnt_adr_sz;
  socklen_t clnt_adr_sz;

  pkt pkt_sender;

  recv_seg_count = 0;
  sender_receive_done = 0;
  send_complete_count = 0;

  /*=====================================>
  LISTEN
  <=====================================*/
  serv_sock = socket(PF_INET, SOCK_STREAM, 0);

  memset(&serv_adr, 0, sizeof(serv_adr));
  serv_adr.sin_family = AF_INET;
  serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
  serv_adr.sin_port = htons(atoi(argv[2]));

  if (bind(serv_sock, (struct sockaddr *)&serv_adr, sizeof(serv_adr)) == -1)
    error_handling("bind() error");
  if (listen(serv_sock, 10) == -1)
    error_handling("listen() error");

  sender_sock = socket(PF_INET, SOCK_STREAM, 0);

  memset(&sender_adr, 0, sizeof(sender_adr));
  sender_adr.sin_family = AF_INET;
  sender_adr.sin_addr.s_addr = inet_addr(argv[5]);
  sender_adr.sin_port = htons(atoi(argv[6]));

  if (connect(sender_sock, (struct sockaddr *)&sender_adr, sizeof(sender_adr)) == -1)
    error_handling("connect() error");

  my_listen_port = htons(atoi(argv[2]));
  if (write_all(sender_sock, &my_listen_port, sizeof(my_listen_port)) == -1)
    error_handling("write() error");

  if (read_exact(sender_sock, &recv_number, sizeof(recv_number)) == -1)
    error_handling("read() error");
  if (read_exact(sender_sock, &my_idx, sizeof(my_idx)) == -1)
    error_handling("read() error");
  if (read_exact(sender_sock, &total_seg_number, sizeof(total_seg_number)) == -1)
    error_handling("read() error");
  if (read_exact(sender_sock, &default_seg_size, sizeof(default_seg_size)) == -1)
    error_handling("read() error");
  if (read_exact(sender_sock, &file_size, sizeof(file_size)) == -1)
    error_handling("read() error");
  if (read_exact(sender_sock, &file_name_len, sizeof(file_name_len)) == -1)
    error_handling("read() error");
  if (read_exact(sender_sock, file_name, file_name_len) == -1)
    error_handling("read() error");
  if (file_name_len >= 0 && (size_t)file_name_len < sizeof(file_name))
    file_name[file_name_len] = '\0';

  total_bytes_recv = 0;
  sender_bytes_recv = 0;
  peer_bytes_recv = malloc(sizeof(long long) * recv_number);
  peer_start_time = malloc(sizeof(double) * recv_number);
  if (peer_bytes_recv == NULL || peer_start_time == NULL)
    error_handling("malloc() error");
  for (int i = 0; i < recv_number; i++)
  {
    peer_bytes_recv[i] = 0;
    peer_start_time[i] = 0.0;
  }
  pthread_mutex_init(&recv_mutex, NULL);
  pthread_mutex_init(&recv_print_mutex, NULL);

  each_seg_size_r = malloc(sizeof(int) * total_seg_number);
  segment = malloc(total_seg_number * sizeof(char *));
  for (int i = 0; i < total_seg_number; i++)
  {
    each_seg_size_r[i] = 0;
    segment[i] = malloc(default_seg_size * sizeof(char));
    for (int j = 0; j < default_seg_size; j++)
      segment[i][j] = '\0';
  }
  recv_info.IP = malloc(sizeof(int) * recv_number);
  recv_info.PORT = malloc(sizeof(short) * recv_number);

  for (int i = 0; i < recv_number; i++)
  {
    if (read_exact(sender_sock, &recv_info.IP[i], sizeof(recv_info.IP[i])) == -1)
      error_handling("read() error");
    if (read_exact(sender_sock, &recv_info.PORT[i], sizeof(recv_info.PORT[i])) == -1)
      error_handling("read() error");
  }

  recv_info_flag = 1;

  for (int i = 0; i < my_idx; i++)
  {
    int *send_arg;
    int *recv_arg;

    clnt_adr_sz = sizeof(clnt_adr);
    clnt_sock = accept(serv_sock, (struct sockaddr *)&clnt_adr, &clnt_adr_sz);
    send_arg = malloc(sizeof(int));
    recv_arg = malloc(sizeof(int));
    if (send_arg == NULL || recv_arg == NULL)
      error_handling("malloc() error");
    *send_arg = clnt_sock;
    *recv_arg = clnt_sock;
    pthread_create(&send_thread, NULL, receiver_send, send_arg);
    pthread_create(&recv_thread, NULL, receiver_recv, recv_arg);
    pthread_detach(send_thread);
    pthread_detach(recv_thread);
  }

  for (int i = my_idx + 1; i < recv_number; i++)
  {
    int *send_arg;
    int *recv_arg;
    clnt_sock = socket(PF_INET, SOCK_STREAM, 0);
    memset(&peer_adr, 0, sizeof(peer_adr));
    peer_adr.sin_family = AF_INET;
    peer_adr.sin_addr.s_addr = recv_info.IP[i];
    peer_adr.sin_port = htons(recv_info.PORT[i]);
    if (connect(clnt_sock, (struct sockaddr *)&peer_adr, sizeof(peer_adr)) == -1)
      error_handling("connect() error");
    send_arg = malloc(sizeof(int));
    recv_arg = malloc(sizeof(int));
    if (send_arg == NULL || recv_arg == NULL)
      error_handling("malloc() error");
    *send_arg = clnt_sock;
    *recv_arg = clnt_sock;
    pthread_create(&send_thread, NULL, receiver_send, send_arg);
    pthread_create(&recv_thread, NULL, receiver_recv, recv_arg);
    pthread_detach(send_thread);
    pthread_detach(recv_thread);
  }
  if (write_all(sender_sock, &recv_info_flag, sizeof(recv_info_flag)) == -1)
    error_handling("write() error");

  system("clear");
  receiver_start_time = receiver_get_time_sec();
  sender_start_time = receiver_start_time;
  receiver_print_status();

  //  Sender애개 데이터 받아옴
  if (read_exact(sender_sock, &my_seg_count, sizeof(my_seg_count)) == -1)
    error_handling("read() error");

  for (int i = 0; i < my_seg_count; i++)
  {
    total_recv = 0;
    // Seg IDX
    if (read_exact(sender_sock, &seg_no, sizeof(seg_no)) == -1)
      error_handling("read() error");
    if (read_exact(sender_sock, &seg_size, sizeof(seg_size)) == -1)
      error_handling("read() error");
    each_seg_size_r[seg_no] = seg_size;
    while (1)
    {
      if (read_exact(sender_sock, &pkt_sender.size, sizeof(pkt_sender.size)) == -1)
        error_handling("read() error");
      if (pkt_sender.size <= 0 || pkt_sender.size > (int)sizeof(pkt_sender.data))
        error_handling("packet size error");
      if (read_exact(sender_sock, pkt_sender.data, pkt_sender.size) == -1)
        error_handling("read() error");
      memcpy(segment[seg_no] + total_recv, pkt_sender.data, pkt_sender.size);
      total_recv += pkt_sender.size;
      receiver_add_sender_bytes(pkt_sender.size);
      if (total_recv >= seg_size)
        break;
    }
    recv_seg_count++;
  }
  sender_receive_done = 1;
  while (total_seg_number > recv_seg_count)
    usleep(1000);
  while (1)
  {
    int done;

    pthread_mutex_lock(&recv_mutex);
    done = send_complete_count;
    pthread_mutex_unlock(&recv_mutex);
    if (done >= recv_number - 1)
      break;
    usleep(1000);
  }

  // 합치기
  fp = fopen(file_name, "wb");
  for (int i = 0; i < total_seg_number; i++)
  {
    fwrite((void *)segment[i], 1, each_seg_size_r[i], fp);
  }
  fclose(fp);
  close(sender_sock);
}

void *receiver_send(void *arg)
{
  int clnt_sock = *((int *)arg);
  free(arg);
  int ptr = 0;
  pkt pkt_r_send;

  while (!sender_receive_done)
    usleep(1000);

  if (write_all(clnt_sock, &my_seg_count, sizeof(my_seg_count)) == -1)
    error_handling("write() error");
  if (write_all(clnt_sock, &my_idx, sizeof(my_idx)) == -1)
    error_handling("write() error");

  for (int i = 0; i < total_seg_number; i++)
  {
    if (i % recv_number == my_idx)
    {
      // Segment IDX
      if (write_all(clnt_sock, &i, sizeof(i)) == -1)
        error_handling("write() error");
      // TOTAL SIZE
      if (write_all(clnt_sock, &each_seg_size_r[i], sizeof(each_seg_size_r[i])) == -1)
        error_handling("write() error");
      ptr = 0;
      while (1)
      {
        if (each_seg_size_r[i] - ptr > 1000)
        {
          pkt_r_send.size = 1000;
          memcpy(pkt_r_send.data, segment[i] + ptr, 1000);
          ptr += 1000;
        }
        else
        {
          pkt_r_send.size = each_seg_size_r[i] - ptr;
          memcpy(pkt_r_send.data, segment[i] + ptr, pkt_r_send.size);
          ptr += pkt_r_send.size;
        }
        if (write_all(clnt_sock, &pkt_r_send.size, sizeof(pkt_r_send.size)) == -1)
          error_handling("write() error");
        if (write_all(clnt_sock, pkt_r_send.data, pkt_r_send.size) == -1)
          error_handling("write() error");
        if (ptr >= each_seg_size_r[i])
          break;
      }
    }
  }
  pthread_mutex_lock(&recv_mutex);
  send_complete_count++;
  pthread_mutex_unlock(&recv_mutex);

  return NULL;
}

void *receiver_recv(void *arg)
{
  int clnt_sock = *((int *)arg);
  free(arg);
  int total_recv;
  int seg_no, seg_size;
  int my_seg_count_r;
  int peer_idx;

  pkt pkt_receiver;

  if (read_exact(clnt_sock, &my_seg_count_r, sizeof(my_seg_count_r)) == -1)
    error_handling("read() error");
  if (read_exact(clnt_sock, &peer_idx, sizeof(peer_idx)) == -1)
    error_handling("read() error");
  if (peer_idx >= 0 && peer_idx < recv_number)
    peer_start_time[peer_idx] = receiver_get_time_sec();

  for (int i = 0; i < my_seg_count_r; i++)
  {
    total_recv = 0;
    // Seg IDX
    if (read_exact(clnt_sock, &seg_no, sizeof(seg_no)) == -1)
      error_handling("read() error");
    if (read_exact(clnt_sock, &seg_size, sizeof(seg_size)) == -1)
      error_handling("read() error");
    each_seg_size_r[seg_no] = seg_size;
    while (1)
    {
      if (read_exact(clnt_sock, &pkt_receiver.size, sizeof(pkt_receiver.size)) == -1)
        error_handling("read() error");
      if (pkt_receiver.size <= 0 || pkt_receiver.size > (int)sizeof(pkt_receiver.data))
        error_handling("packet size error");
      if (read_exact(clnt_sock, pkt_receiver.data, pkt_receiver.size) == -1)
        error_handling("read() error");
      memcpy(segment[seg_no] + total_recv, pkt_receiver.data, pkt_receiver.size);
      total_recv += pkt_receiver.size;
      receiver_add_peer_bytes(peer_idx, pkt_receiver.size);
      if (total_recv >= seg_size)
        break;
    }
    recv_seg_count++;
  }

  return NULL;
}

void receiver_add_sender_bytes(int size)
{
  pthread_mutex_lock(&recv_mutex);
  sender_bytes_recv += size;
  total_bytes_recv += size;
  pthread_mutex_unlock(&recv_mutex);

  receiver_print_status();
}

void receiver_add_peer_bytes(int peer_idx, int size)
{
  if (peer_idx < 0 || peer_idx >= recv_number)
    return;

  pthread_mutex_lock(&recv_mutex);
  peer_bytes_recv[peer_idx] += size;
  total_bytes_recv += size;
  pthread_mutex_unlock(&recv_mutex);

  receiver_print_status();
}

void receiver_print_status(void)
{
  char process[11];
  int proc = 0;
  int bar_count;
  int line;
  double now;
  double elapsed_time;
  double mbps;
  double sender_elapsed;
  double sender_mbps;

  pthread_mutex_lock(&recv_mutex);

  now = receiver_get_time_sec();
  if (file_size > 0)
    proc = (int)((100LL * total_bytes_recv) / file_size);
  if (proc > 100)
    proc = 100;
  bar_count = proc / 10;

  for (int i = 0; i < 10; i++)
    process[i] = ' ';
  process[10] = '\0';
  for (int i = 0; i < bar_count; i++)
    process[i] = '#';

  elapsed_time = now - receiver_start_time;
  if (elapsed_time > 0)
    mbps = ((double)total_bytes_recv * 8.0) / elapsed_time / 1000000.0;
  else
    mbps = 0.0;

  sender_elapsed = now - sender_start_time;
  if (sender_elapsed > 0)
    sender_mbps = ((double)sender_bytes_recv * 8.0) / sender_elapsed / 1000000.0;
  else
    sender_mbps = 0.0;

  pthread_mutex_lock(&recv_print_mutex);

  printf("\033[1;1H");
  printf("\033[K");
  printf("Receiving Peer %d [%s] %d%% (%lld/%lldBytes) %.2fMbps (%.1fs)\n",
         my_idx + 1, process, proc, total_bytes_recv, file_size, mbps, elapsed_time);

  printf("\033[2;1H");
  printf("\033[K");
  printf("From Sending Peer : %.2fMbps (%lld Bytes Sent / %.1fs)\n",
         sender_mbps, sender_bytes_recv, sender_elapsed);

  line = 3;
  for (int i = 0; i < recv_number; i++)
  {
    double peer_elapsed = 0.0;
    double peer_mbps = 0.0;

    if (i == my_idx)
      continue;
    if (peer_start_time[i] > 0)
      peer_elapsed = now - peer_start_time[i];
    if (peer_elapsed > 0)
      peer_mbps = ((double)peer_bytes_recv[i] * 8.0) / peer_elapsed / 1000000.0;

    printf("\033[%d;1H", line);
    printf("\033[K");
    printf("From Receiving Peer #%d: %.2fMbps (%lld Bytes Sent / %.1fs)\n",
           i + 1, peer_mbps, peer_bytes_recv[i], peer_elapsed);
    line++;
  }

  fflush(stdout);
  pthread_mutex_unlock(&recv_print_mutex);
  pthread_mutex_unlock(&recv_mutex);
}

double receiver_get_time_sec(void)
{
  struct timespec ts;

  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (double)ts.tv_sec + ((double)ts.tv_nsec / 1000000000.0);
}