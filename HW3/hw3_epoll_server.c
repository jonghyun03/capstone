#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/epoll.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>

#define BUF_SIZE 1024
#define EPOLL_SIZE 50
void error_handling(char *buf);

int main(int argc, char *argv[])
{
  char pwd[EPOLL_SIZE][100];
  char orig_pwd[100];

  int invalid = -1;

  int serv_sock, clnt_sock;
  struct sockaddr_in serv_adr, clnt_adr;
  char buf[BUF_SIZE];
  char tmp[BUF_SIZE];
  struct timeval timeout;
  fd_set reads, cpy_reads, writes;

  int len;

  FILE *fp;
  int read_cnt;

  DIR *mydir;
  struct dirent *myfile;
  struct stat mystat;

  socklen_t adr_sz;
  int fd_max, str_len, fd_num, i;

  struct epoll_event *ep_events;
  struct epoll_event event;
  int epfd, event_cnt;

  if (argc != 2)
  {
    printf("Usage : %s <port>\n", argv[0]);
    exit(1);
  }

  // PWD 설정
  getcwd(orig_pwd, sizeof(orig_pwd));
  for (i = 0; i < EPOLL_SIZE; i++)
    getcwd(pwd[i], sizeof(pwd[i]));


  // 서버 소켓 설정
  serv_sock = socket(PF_INET, SOCK_STREAM, 0);
  memset(&serv_adr, 0, sizeof(serv_adr));
  serv_adr.sin_family = AF_INET;
  serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
  serv_adr.sin_port = htons(atoi(argv[1]));

  if (bind(serv_sock, (struct sockaddr *)&serv_adr, sizeof(serv_adr)) == -1)
    error_handling("bind() error");
  if (listen(serv_sock, 5) == -1)
    error_handling("listen() error");

  epfd = epoll_create(EPOLL_SIZE);
  ep_events = malloc(sizeof(struct epoll_event) * EPOLL_SIZE);

  event.events = EPOLLIN;
  event.data.fd = serv_sock;
  epoll_ctl(epfd, EPOLL_CTL_ADD, serv_sock, &event);

  while (1)
  {

    if ((event_cnt = epoll_wait(epfd, ep_events, EPOLL_SIZE, -1)) == -1)
    {
      puts("epoll_wait() error");
      break;
    }

    for (i = 0; i < event_cnt; i++)
    {
      if (ep_events[i].data.fd == serv_sock)
      {
        adr_sz = sizeof(clnt_adr);
        clnt_sock = accept(serv_sock, (struct sockaddr *)&clnt_adr, &adr_sz);
        write(clnt_sock, pwd[clnt_sock], strlen(pwd[ep_events[i].data.fd]));
        event.events = EPOLLIN;
        event.data.fd = clnt_sock;
        epoll_ctl(epfd, EPOLL_CTL_ADD, clnt_sock, &event);
        printf("connected client: %d \n", clnt_sock);
      }
      else
      {
        memset(buf, 0, sizeof(buf));
        read_cnt = read(ep_events[i].data.fd, buf, BUF_SIZE);
        if (read_cnt == 0)
        {
          epoll_ctl(epfd, EPOLL_CTL_DEL, ep_events[i].data.fd, NULL);
          close(ep_events[i].data.fd);
          memset(pwd[ep_events[i].data.fd], 0, sizeof(pwd[ep_events[i].data.fd]));
          strcpy(pwd[ep_events[i].data.fd], orig_pwd);
          printf("closed client: %d \n", ep_events[i].data.fd);
        }
        // 해당 클라이언트가 접속중인 디렉토리로 이동
        chdir(pwd[ep_events[i].data.fd]);
        //
        //
        if (strncmp(buf, "cd ", 3) == 0)
        { // Change Directory
          char *ptr = strtok(buf, " ");
          ptr = strtok(NULL, " ");
          if (chdir(ptr) == 0)
          { // cd 성공
            getcwd(pwd[ep_events[i].data.fd], sizeof(pwd[ep_events[i].data.fd]));
            memset(buf, 0, sizeof(buf));
            sprintf(buf, "%s\n", pwd[ep_events[i].data.fd]);
            len = strlen(buf);
            len = htons(len);
            write(ep_events[i].data.fd, (int *)&len, sizeof(len));
            write(ep_events[i].data.fd, buf, strlen(buf));
            memset(buf, 0, sizeof(buf));
          }
          else
          {
            memset(tmp, 0, sizeof(tmp));
            sprintf(tmp, "cd: no such file or directory: %s\n%s\n", ptr, pwd[ep_events[i].data.fd]);
            write(ep_events[i].data.fd, buf, strlen(buf));
            memset(tmp, 0, sizeof(tmp));
            memset(buf, 0, sizeof(buf));
          }
        }
        else if (strcmp(buf, "ls") == 0)
        { // LS
          memset(buf, 0, sizeof(buf));
          mydir = opendir(".");
          while ((myfile = readdir(mydir)) != NULL)
          {
            stat(myfile->d_name, &mystat);
            switch (mystat.st_mode & S_IFMT)
            {
            case S_IFBLK:
              strcat(buf, "block device\t");
              break;
            case S_IFCHR:
              strcat(buf, "character device\t");
              break;
            case S_IFDIR:
              strcat(buf, "directory\t");
              break;
            case S_IFIFO:
              strcat(buf, "FIFO/pipe\t");
              break;
            case S_IFLNK:
              strcat(buf, "symlink\t");
              break;
            case S_IFREG:
              strcat(buf, "regular file\t");
              break;
            case S_IFSOCK:
              strcat(buf, "socket\t");
              break;
            default:
              strcat(buf, "unknown?\t");
              break;
            }
            sprintf(tmp, "%ld", mystat.st_size);
            strcat(buf, tmp);
            strcat(buf, "\t");
            strcat(buf, myfile->d_name);
            strcat(buf, "\n");
          }
          strcat(buf, pwd[ep_events[i].data.fd]);
          strcat(buf, "\n");
          closedir(mydir);
          len = strlen(buf);
          len = htons(len);
          write(ep_events[i].data.fd, (int *)&len, sizeof(len));
          write(ep_events[i].data.fd, buf, strlen(buf));
          memset(buf, 0, sizeof(buf));
          memset(tmp, 0, sizeof(tmp));
        }
        else
        { // file download
          printf("Client request: %s\n", buf);
          fp = fopen(buf, "rb");
          if (fp == NULL)
          {
            memset(buf, 0, sizeof(buf));
            write(ep_events[i].data.fd, (int *)&invalid, sizeof(invalid));
            strcpy(buf, pwd[ep_events[i].data.fd]);
            strcat(buf, "\n");
            len = strlen(buf);
            len = htons(len);
            write(ep_events[i].data.fd, (int *)&len, sizeof(len));
            write(ep_events[i].data.fd, buf, strlen(buf));
            memset(buf, 0, sizeof(buf));
          }
          else
          {
            // 파일 잘 찾음
            stat(buf, &mystat);
            len = (int)mystat.st_size;
            write(ep_events[i].data.fd, (int *)&len, sizeof(len));
            while (1)
            {
              memset(buf, 0, sizeof(buf));
              read_cnt = fread((void *)buf, 1, BUF_SIZE, fp);
              if (read_cnt < BUF_SIZE)
              {
                write(ep_events[i].data.fd, buf, read_cnt);
                break;
              }
              write(ep_events[i].data.fd, buf, BUF_SIZE);
            }
            fclose(fp);
            memset(buf, 0, sizeof(buf));
            sprintf(buf, "%s\n", pwd[ep_events[i].data.fd]);
            write(ep_events[i].data.fd, buf, strlen(buf));
            memset(buf, 0, sizeof(buf));
          }
        }
      }
    }
  }
  close(serv_sock);
  return 0;
}

void error_handling(char *buf)
{
  fputs(buf, stderr);
  fputc('\n', stderr);
  exit(1);
}