#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define COLOR_ORANGE "\033[38;2;255;140;0m"
#define COLOOR_CYAN "\033[38;2;0;255;255m"
#define COLOR_RESET "\033[0m"

#define BUF_SIZE 1024
void error_handling(char *message);

int main(int argc, char *argv[])
{
  int sd, i, k;
  int errchk = 0;
  FILE *fp;

  char buf[BUF_SIZE];
  char file_name[BUF_SIZE];
  char file_list[BUF_SIZE];

  int len;

  char size[9];
  int read_cnt;
  int size_cnt = 0;
  int size_int;
  struct sockaddr_in serv_adr;
  if (argc != 3)
  {
    printf("Usage: %s <IP> <port>\n", argv[0]);
    exit(1);
  }

  sd = socket(PF_INET, SOCK_STREAM, 0);

  memset(&serv_adr, 0, sizeof(serv_adr));
  serv_adr.sin_family = AF_INET;
  serv_adr.sin_addr.s_addr = inet_addr(argv[1]);
  serv_adr.sin_port = htons(atoi(argv[2]));

  connect(sd, (struct sockaddr *)&serv_adr, sizeof(serv_adr));
  read_cnt = read(sd, buf, BUF_SIZE);
  // PWD
  printf("%s\n", buf);
  while (1)
  {
    memset(buf, 0, sizeof(buf));
    fgets(buf, BUF_SIZE, stdin);
    buf[strlen(buf) - 1] = '\0';
    if (strcmp(buf, "exit") == 0)
    {
      close(sd);
      printf("%sGood bye...%s\n", COLOOR_CYAN, COLOR_RESET);
      return 0;
    }
    else if (strncmp(buf, "cd ", 3) == 0)
    {
      write(sd, buf, strlen(buf));
      memset(buf, 0, sizeof(buf));
      read(sd, (int *)&len, sizeof(len));
      len = ntohl(len);
      size_cnt = 0;
      memset(file_list, 0, sizeof(file_list));
      while (size_cnt < len)
      {
        read_cnt = read(sd, file_name, BUF_SIZE);
        if (read_cnt == -1)
          error_handling("read() error...");
        size_cnt += read_cnt;
        strcat(file_list, file_name);
      }
      printf("%s", file_list);
    }
    else if (strcmp(buf, "ls") == 0)
    {
      write(sd, buf, strlen(buf));
      memset(buf, 0, sizeof(buf));
      read(sd, (int *)&len, sizeof(len));
      len = ntohl(len);
      size_cnt = 0;
      memset(file_list, 0, sizeof(file_list));
      memset(file_name, 0, sizeof(file_name));
      while (size_cnt < len)
      {
        read_cnt = read(sd, file_name, BUF_SIZE);
        if (read_cnt == -1)
          error_handling("read() error...");
        size_cnt += read_cnt;
        strcat(file_list, file_name);
      }
      printf("%s", file_list);
      memset(file_list, 0, sizeof(file_list));
      memset(file_name, 0, sizeof(file_name));
    }
    else
    { // file download
      write(sd, buf, strlen(buf));
      read(sd, (int *)&len, sizeof(len));
      len = ntohl(len);
      if (len == -1)
      {
        memset(buf, 0, sizeof(buf));
        // Wrong Command
        printf("%sWrong command... no such file or command.%s\n", COLOR_ORANGE, COLOR_RESET);
        read(sd, (int *)&len, sizeof(len));
        len = ntohl(len);
        size_cnt = 0;
        memset(file_list, 0, sizeof(file_list));
        memset(file_name, 0, sizeof(file_name));
        while (size_cnt < len)
        {
          read_cnt = read(sd, file_name, BUF_SIZE);
          if (read_cnt == -1)
            error_handling("read() error...");
          size_cnt += read_cnt;
          strcat(file_list, file_name);
        }
        // PWD 출력
        printf("%s", file_list);
        continue;
      }

      // 파일 찾음
      size_cnt = 0;
      memset(file_list, 0, sizeof(file_list));
      memset(file_name, 0, sizeof(file_name));
      fp = fopen(buf, "wb");
      memset(buf, 0, sizeof(buf));
      while (size_cnt < len)
      {
        if ((len - size_cnt) > BUF_SIZE)
        {
          read_cnt = read(sd, buf, BUF_SIZE);
        }
        else
        { // 마지막 읽을 때
          read_cnt = read(sd, buf, len - size_cnt);
        }
        if (read_cnt == -1)
          error_handling("read() error...");
        size_cnt += read_cnt;
        // printf("%d-----%d-----\n", len, size_cnt);
        fwrite((void *)buf, 1, read_cnt, fp);
      }
      fclose(fp);
      memset(buf, 0, sizeof(buf));
      read(sd, buf, BUF_SIZE);
      printf("%s", buf);
      memset(buf, 0, sizeof(buf));
    }
  }
  close(sd);
  return 0;
}

void error_handling(char *message)
{
  fputs(message, stderr);
  fputc('\n', stderr);
  exit(1);
}