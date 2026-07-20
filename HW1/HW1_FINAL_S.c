#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>

#define BUF_SIZE 30
void error_handling(char *message);

int main(int argc, char *argv[])
{
  int serv_sd, clnt_sd, i, len, k;
  FILE *fp;
  char buf[BUF_SIZE];
  int read_cnt;
  DIR *mydir;
  struct dirent *myfile;
  struct stat mystat;
  int read_len, flist_len;
  char file_list[1024];
  char size[9];
  char file_name[30];
  char file_size[30];

  char tmp[30];

  long length;

  struct sockaddr_in serv_adr, clnt_adr;
  socklen_t clnt_adr_sz;

  if (argc != 2)
  {
    printf("Usage: %s <port>\n", argv[0]);
    exit(1);
  }

  serv_sd = socket(PF_INET, SOCK_STREAM, 0);

  memset(&serv_adr, 0, sizeof(serv_adr));
  serv_adr.sin_family = AF_INET;
  serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
  serv_adr.sin_port = htons(atoi(argv[1]));

  bind(serv_sd, (struct sockaddr *)&serv_adr, sizeof(serv_adr));
  listen(serv_sd, 5);

  clnt_adr_sz = sizeof(clnt_adr);

  // ls cmd
  mydir = opendir(".");
  while ((myfile = readdir(mydir)) != NULL)
  {
    if (strncmp(myfile->d_name, ".", 1) == 0)
      continue;
    stat(myfile->d_name, &mystat);
    if((mystat.st_mode & S_IFMT) != S_IFREG)
      continue;
    memset(tmp, 0, sizeof(tmp));
    sprintf(tmp, "%ld ", mystat.st_size);
    strcat(file_list, tmp);
    strcat(file_list, myfile->d_name);
    strcat(file_list, "\n");
  }
  closedir(mydir);

  printf("file list:\n%s", file_list);

  clnt_sd = accept(serv_sd, (struct sockaddr *)&clnt_adr, &clnt_adr_sz);
  printf("Accpet success...\n\n\n");
  for (k = 0; k < 2; k++)
  {
    // 보낼 내용 크기 구하기
    length = strlen(file_list);
    length = htonl(length);

    // 크기 보내기 (8바이트)
    write(clnt_sd, (int*)&length, sizeof(length));
    printf("File list size sent: %d\n", ntohl(length));
    // 내용 보내기
    write(clnt_sd, file_list, strlen(file_list)+1);

    read_len = read(clnt_sd, file_name, BUF_SIZE);
    printf("REQ -- file [%s] has been requested\n", file_name);
    // 파일 찾기
    if (stat(file_name, &mystat) == -1)
    {
      length = htonl(-1);
      write(clnt_sd, (int*)&length, sizeof(length));
      close(clnt_sd);
      close(serv_sd);
      error_handling("클라이언트의 잘못된 요청으로 인한 프로그램 종료.");
      return -1;
    }
    length = mystat.st_size;
    length = htonl(length);

    // 크기 보내기
    write(clnt_sd, (int*)&length, sizeof(length));
    printf("SIZE [%ld] had been sent\n", length);

    file_name[read_len - 1] = '\0';

    fp = fopen(file_name, "rb");

    while (1)
    {
      memset(buf, 0, sizeof(buf));
      read_cnt = fread((void *)buf, 1, BUF_SIZE, fp);
      if (read_cnt < BUF_SIZE)
      {
        write(clnt_sd, buf, read_cnt);
        break;
      }

      write(clnt_sd, buf, BUF_SIZE);
    }
    printf("file [%s] has been sent\n", file_name);

    // shutdown(clnt_sd, SHUT_WR);
    read(clnt_sd, buf, BUF_SIZE);
    printf("Message from client: %s \n", buf);
    printf("\n|--------------------------|\n\n");

    fclose(fp);
  }
  close(clnt_sd);
  close(serv_sd);
  return 0;
}

void error_handling(char *message)
{
  fputs(message, stderr);
  fputc('\n', stderr);
  exit(1);
}
