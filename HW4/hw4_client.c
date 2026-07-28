// ./a.out 172.17.0.2 9190 haha
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>
#include <semaphore.h>
#include <termios.h>

#define BUF_SIZE 100
#define NAME_SIZE 20

#define COLOR_GREEN "\033[38;2;0;255;0m"
#define COLOR_RESET "\033[0m"

void *send_msg(void *arg);
void *recv_msg(void *arg);
void error_handling(char *msg);
char getch();

char name[NAME_SIZE] = "[DEFAULT]";
char msg[BUF_SIZE];

static sem_t sem_snd;
static sem_t sem_rcv;
static int num;

int main(int argc, char *argv[])
{
  int sock;
  struct sockaddr_in serv_addr;
  pthread_t snd_thread, rcv_thread;
  sem_init(&sem_snd, 0, 0);
  sem_init(&sem_rcv, 0, 1);
  void *thread_return;
  if (argc != 4)
  {
    printf("Usage : %s <IP> <port> <name>\n", argv[0]);
    exit(1);
  }

  system("clear");

  sprintf(name, "[%s]", argv[3]);
  sock = socket(PF_INET, SOCK_STREAM, 0);

  memset(&serv_addr, 0, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
  serv_addr.sin_port = htons(atoi(argv[2]));

  if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
    error_handling("connect() error");

  pthread_create(&snd_thread, NULL, send_msg, (void *)&sock);
  pthread_create(&rcv_thread, NULL, recv_msg, (void *)&sock);
  pthread_join(snd_thread, &thread_return);
  pthread_join(rcv_thread, &thread_return);

  sem_destroy(&sem_snd);
  sem_destroy(&sem_rcv);
  close(sock);
  return 0;
}

void *send_msg(void *arg) // send thread main
{
  int sock = *((int *)arg);
  char ch;
  char msg[BUF_SIZE];
  memset(msg, 0, sizeof(msg));
  printf("%sSearch Word: %s", COLOR_GREEN, COLOR_RESET);
  fflush(stdout);
  while (1)
  {
    ch = getch();
    if (ch == 10)
    { // enter
      close(sock);
      exit(0);
    }
    else if (ch == 127)
    { // backspace
      if (strlen(msg) >= 1)
        msg[strlen(msg) - 1] = '\0';
    }
    else if ((ch >= 20) && (ch <= 126))
    {
      msg[strlen(msg)] = ch;
    }
    printf("\033[1;1H"); // 제일 위에 줄 제일 좌측으로 커서 이동
    printf("\r\033[K"); // 현재 줄 삭제
    fflush(stdout);
    printf("%sSearch Word: %s", COLOR_GREEN, COLOR_RESET);
    printf("%s\n", msg);
    printf("------------------------------\n");
    fflush(stdout);
    tcflush(0, TCIFLUSH); // 입력 버퍼 비우기

    sem_wait(&sem_rcv);
    if (strlen(msg) > 0)
      write(sock, msg, strlen(msg));
    else
      write(sock, msg, 1);
    sem_post(&sem_snd);
  }
  return NULL;
}

void *recv_msg(void *arg) // read thread main
{
  int sock = *((int *)arg);
  char name_msg[1024];
  int str_len;

  while (1)
  {
    sem_wait(&sem_snd);
    memset(name_msg, 0, sizeof(name_msg));
    printf("\033[3;1H\033[J");  // 3행 1열로 이동 후 뒤에 모두 제거
    fflush(stdout);
    str_len = read(sock, name_msg, 1024);
    if (str_len == -1)
      return (void *)-1;
    // name_msg[strlen(name_msg)] = 0;
    // printf(">==%d==<\n", strlen(name_msg));
    fputs(name_msg, stdout);
    fflush(stdout);
    sem_post(&sem_rcv);
  }

  return NULL;
}

void error_handling(char *msg)
{
  fputs(msg, stderr);
  fputc('\n', stderr);
  exit(1);
}

char getch()
{
  char buf = 0;
  struct termios old = {0};

  // 기존 터미널 설정 저장
  if (tcgetattr(STDIN_FILENO, &old) < 0)
    perror("tcgetattr");

  // 캐논 모드 및 에코 비활성화
  old.c_lflag &= ~ICANON; // 캐논 모드 비활성화
  old.c_lflag &= ~ECHO;   // 에코 비활성화
  old.c_cc[VMIN] = 1;     // 최소 1바이트 입력 대기
  old.c_cc[VTIME] = 0;    // 입력 대기 시간 설정

  if (tcsetattr(STDIN_FILENO, TCSANOW, &old) < 0)
    perror("tcsetattr");

  // 한 글자 읽기
  if (read(STDIN_FILENO, &buf, 1) < 0)
    perror("read");

  // printf("아스키코드 (10진수): %d\n", buf);

  // 원래 설정 복구
  old.c_lflag |= ICANON;
  old.c_lflag |= ECHO;
  if (tcsetattr(STDIN_FILENO, TCSADRAIN, &old) < 0)
    perror("tcsetattr");

  return buf;
}