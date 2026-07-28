#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <sys/stat.h>

#define BUF_SIZE 100
#define MAX_CLNT 256

#define COLOR_ORANGE "\033[38;2;255;140;0m"
#define COLOR_RESET "\033[0m"

void *handle_clnt(void *arg);
void send_msg(char *msg, int len, int fd);
void error_handling(char *msg);
void bubble_sort(int list[], int n, char clist[][200]);

int clnt_cnt = 0;
int clnt_socks[MAX_CLNT];
pthread_mutex_t mutx;

int main(int argc, char *argv[])
{
  int serv_sock, clnt_sock;
  struct sockaddr_in serv_adr, clnt_adr;
  int clnt_adr_sz;
  pthread_t t_id;
  if (argc != 2)
  {
    printf("Usage : %s <port>\n", argv[0]);
    exit(1);
  }

  pthread_mutex_init(&mutx, NULL);
  serv_sock = socket(PF_INET, SOCK_STREAM, 0);

  memset(&serv_adr, 0, sizeof(serv_adr));
  serv_adr.sin_family = AF_INET;
  serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
  serv_adr.sin_port = htons(atoi(argv[1]));

  if (bind(serv_sock, (struct sockaddr *)&serv_adr, sizeof(serv_adr)) == -1)
    error_handling("bind() error");
  if (listen(serv_sock, 5) == -1)
    error_handling("listen() error");

  while (1)
  {
    clnt_adr_sz = sizeof(clnt_adr);
    clnt_sock = accept(serv_sock, (struct sockaddr *)&clnt_adr, &clnt_adr_sz);

    pthread_mutex_lock(&mutx);
    clnt_socks[clnt_cnt++] = clnt_sock;
    pthread_mutex_unlock(&mutx);

    pthread_create(&t_id, NULL, handle_clnt, (void *)&clnt_sock);
    pthread_detach(t_id);
    printf("Connected client IP: %s \n", inet_ntoa(clnt_adr.sin_addr));
  }
  close(serv_sock);
  return 0;
}

void *handle_clnt(void *arg)
{
  int clnt_sock = *((int *)arg);
  int str_len = 0, i;
  char msg[BUF_SIZE];

  while (1)
  {
    memset(msg, 0, sizeof(msg));
    str_len = read(clnt_sock, msg, sizeof(msg));
    if (str_len == 0)
      break;
    send_msg(msg, str_len, clnt_sock);
  }

  pthread_mutex_lock(&mutx);
  for (i = 0; i < clnt_cnt; i++) // remove disconnected client
  {
    if (clnt_sock == clnt_socks[i])
    {
      while (i++ < clnt_cnt - 1)
        clnt_socks[i] = clnt_socks[i + 1];
      break;
    }
  }
  clnt_cnt--;
  pthread_mutex_unlock(&mutx);
  close(clnt_sock);
  return NULL;
}
void send_msg(char *msg, int len, int fd) // send to all
{
  FILE *fp;

  struct stat mystat;
  char *str;

  char line[200];
  char freq[20];
  int weight;

  char *ptr1_save = NULL;
  char *ptr3_save = NULL;

  char *ptr1;
  char *ptr2;
  char *ptr3;

  char list[200][200];
  int w_list[200];

  int count = 0;

  int tmp;

  // printf(">===|%s|====<MSG\n", msg);

  stat("data.txt", &mystat);

  memset(list, 0, sizeof(list));

  str = malloc(sizeof(char) * mystat.st_size);

  fp = fopen("data.txt", "rb");

  fread(str, 1, mystat.st_size, fp);

  ptr1 = strtok_r(str, "\n", &ptr1_save);
  while (ptr1 != NULL)
  {
    ptr2 = strstr(ptr1, msg);
    if (ptr2 != NULL)
    {
      memset(line, 0, 200);
      strcpy(line, ptr1);
      int check = 0;
      ptr3 = strtok_r(line, "-", &ptr3_save);
      while (ptr3 != NULL)
      {
        if (check == 0)
        {

          tmp = (int)(ptr2 - ptr1);
          if (tmp == 0)
          {
            strcat(list[count], COLOR_ORANGE);
            strncat(list[count], ptr3, strlen(msg));
            strcat(list[count], COLOR_RESET);
            ptr3 += strlen(msg);
            if (strlen(ptr3) > 0)
              strcat(list[count], ptr3);
          }
          else
          {
            strncat(list[count], ptr3, tmp);
            ptr3 += tmp;
            strcat(list[count], COLOR_ORANGE);
            strncat(list[count], ptr3, strlen(msg));
            strcat(list[count], COLOR_RESET);
            ptr3 += strlen(msg);
            if (strlen(ptr3) > 0)
              strcat(list[count], ptr3);
          }
          check++;
        }
        else
        {
          strcpy(freq, ptr3);
          w_list[count] = atoi(freq);
          count++;
        }
        ptr3 = strtok_r(NULL, "-", &ptr3_save);
      }
    }
    ptr1 = strtok_r(NULL, "\n", &ptr1_save);
  }

  bubble_sort(w_list, count, list);

  char res_list[1024];

  memset(res_list, 0, sizeof(res_list));
  for (int i = 0; i < (count > 10 ? 10 : count); i++)
  {
    // printf("<===%s===>\n", list[i]);
    strcat(res_list, list[i]);
    strcat(res_list, "\n");
  }
  fclose(fp);

  int length;
  length = strlen(res_list);

  pthread_mutex_lock(&mutx);
  if (count == 0)
  {
    write(fd, res_list, 1);
  }
  else
  {
    write(fd, res_list, length);
  }
  pthread_mutex_unlock(&mutx);
  // printf("<%d======%s=======>RES\n", length, res_list);
}
void error_handling(char *msg)
{
  fputs(msg, stderr);
  fputc('\n', stderr);
  exit(1);
}
void bubble_sort(int list[], int n, char clist[][200])
{
  int i, j, temp;

  char tmp[100];

  for (i = n - 1; i > 0; i--)
  {
    for (j = 0; j < i; j++)
    {
      if (list[j] < list[j + 1])
      {
        temp = list[j];
        list[j] = list[j + 1];
        list[j + 1] = temp;

        strcpy(tmp, clist[j]);
        strcpy(clist[j], clist[j + 1]);
        strcpy(clist[j + 1], tmp);
      }
    }
  }
}