#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define BUF_SIZE 30
void error_handling(char *message);

int main(int argc, char *argv[])
{
	int sd, i, k;
  int errchk = 0;
	FILE *fp;

  long len;
	
	char buf[BUF_SIZE];
  char file_name[BUF_SIZE];
  char file_list[1024];
  char size[9];
	int read_cnt;
  int size_cnt = 0;
  int size_int;
	struct sockaddr_in serv_adr;
	if (argc != 3) {
		printf("Usage: %s <IP> <port>\n", argv[0]);
		exit(1);
	}
	
	sd = socket(PF_INET, SOCK_STREAM, 0);   

	memset(&serv_adr, 0, sizeof(serv_adr));
	serv_adr.sin_family = AF_INET;
	serv_adr.sin_addr.s_addr = inet_addr(argv[1]);
	serv_adr.sin_port = htons(atoi(argv[2]));

	connect(sd, (struct sockaddr*)&serv_adr, sizeof(serv_adr));
  // 디렉토리 내 파일 크기 읽어오기
  //
  for (k = 0; k<2; k++)
  {
    read(sd, (long*)&len, sizeof(len));
    len = ntohl(len);

    // file 크기 DEBUG
    // printf("file list size: %ld\n", len);
    
    size_cnt = 0;
    // 파일 리스트 읽기
    // 
    while (size_cnt<len)
    {
      read_cnt=read(sd, file_name, BUF_SIZE );
      if(read_cnt == -1)
        error_handling("read() error...");
      size_cnt += read_cnt;
      strcat(file_list, file_name);
      memset(file_name, 0, sizeof(file_name));
    }
    printf("%s", file_list);
    size_cnt = 0;
    printf("\n\n--Which file do you want?--\n");

    // 읽어올 파일 입력받기
    fgets(file_name, sizeof(file_name), stdin);
    file_name[strlen(file_name)-1] = '\0';

    // 읽고싶은 파일 전송
    // 
    write(sd, file_name, sizeof(file_name));

    // 크기 읽어오기
    // 
    read(sd, (long*)&len, sizeof(len));
    len = ntohl(len);

    if(len == -1){
      close(sd);
      error_handling("No such file Error. Terminating system...\n");
      return -1;
    }
    size_cnt = 0;

    fp = fopen(file_name, "wb");
    while (size_cnt<len)
    {
      read_cnt=read(sd, buf, BUF_SIZE );
      if(read_cnt == -1)
        error_handling("read() error...");
      size_cnt += read_cnt;
      fwrite((void*)buf, 1, read_cnt, fp);
    }
    puts("Received file data\n");
    puts("|--------------------------|\n\n");
    write(sd, "Thank you", 10);
    fclose(fp);
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