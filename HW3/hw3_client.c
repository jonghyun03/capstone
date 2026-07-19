#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

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
      printf("Good bye...\n");
      return 0;
    }
    else if (strncmp(buf, "cd ", 3) == 0)
    {
      write(sd, buf, strlen(buf));
      memset(buf, 0, sizeof(buf));
      read(sd, (int *)&len, sizeof(len));
      len = ntohs(len);
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
      len = ntohs(len);
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
      if (len == -1)
      {
        memset(buf, 0, sizeof(buf));
        // Wrong Command
        printf("Wrong command... no such file or command.\n");
        read(sd, (int *)&len, sizeof(len));
        len = ntohs(len);
        size_cnt = 0;
        memset(file_list, 0, sizeof(file_list));
        memset(file_name, 0, sizeof(file_name));
        printf("HERE???????????.\n");
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
      }

      // 파일 찾음
      size_cnt = 0;
      memset(file_list, 0, sizeof(file_list));
      memset(file_name, 0, sizeof(file_name));
      fp = fopen(buf, "wb");
      memset(buf, 0, sizeof(buf));
      while (size_cnt < len)
      {
        read_cnt = read(sd, buf, len-size_cnt);
        if (read_cnt == -1)
          error_handling("read() error...");
        size_cnt += read_cnt;
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