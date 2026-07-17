#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <time.h>

#define BUF_SIZE 30
void error_handling(char *message);

typedef struct
{
	long time;
	int seq;
	char message[BUF_SIZE];
} pkt_s;

int main(int argc, char *argv[])
{
	char file_name[30];
	int serv_sock;
	int str_len;
	int check;
	int read_cnt;
	int wait_count = 0;
	socklen_t clnt_adr_sz;
	FILE *fp;
	struct stat mystat;
	int first_flag = 0;

	pkt_s *pkt = malloc(sizeof(pkt_s));
	pkt_s *ack = malloc(sizeof(pkt_s));
	time_t cur_time;
	int seq_flag = 0;

	struct sockaddr_in serv_adr, clnt_adr;
	struct timeval optVal = {6, 0};
	int optLen = sizeof(optVal);

	if (argc != 3)
	{
		printf("Usage : %s <IP> <port>\n", argv[0]);
		exit(1);
	}

	serv_sock = socket(PF_INET, SOCK_DGRAM, 0);
	setsockopt(serv_sock, SOL_SOCKET, SO_RCVTIMEO, &optVal, optLen);
	if (serv_sock == -1)
		error_handling("UDP socket creation error");

	memset(&serv_adr, 0, sizeof(serv_adr));
	serv_adr.sin_family = AF_INET;
	serv_adr.sin_addr.s_addr = inet_addr(argv[1]);
	serv_adr.sin_port = htons(atoi(argv[2]));

	pkt->seq = 0;
	ack->seq = -1;

	fputs("Enter the file do you want to send: ", stdout);
	fgets(file_name, sizeof(file_name), stdin);
	file_name[strlen(file_name) - 1] = '\0';
	if (stat(file_name, &mystat) == -1)
	{
		printf("--NULL--\n");
		free(pkt);
		free(ack);
		close(serv_sock);
		error_handling("Wrong input. Terminate...\n");
	}
	fp = fopen(file_name, "rb");
	if (fp == NULL)
	{
		printf("--NULL--\n");
		free(pkt);
		free(ack);
		close(serv_sock);
		error_handling("Wrong input. Terminate...\n");
	}
	printf("FILE NAME: %s\n", file_name);
	while (1)
	{
		if (first_flag == 0)
		{
			// 파일 크기 보내기
			read_cnt = BUF_SIZE + 1;
			sprintf(pkt->message, "%ld", mystat.st_size);
			printf("FILE SIZE: %ld\n", mystat.st_size);
			first_flag++;
		}
		else if (first_flag == 1)
		{
			strcpy(pkt->message, file_name);
			first_flag++;
		}
		else
		{
			// 파일 읽기
			memset(pkt->message, 0, BUF_SIZE);
			read_cnt = fread((void *)pkt->message, 1, BUF_SIZE, fp);
		}

		clnt_adr_sz = sizeof(clnt_adr);
		seq_flag = 0;
		wait_count = 0;
		str_len = -1;

		// 보내고 받기
		while ((str_len == -1 && wait_count < 3) && seq_flag == 0)
		{

			// seq Network 변환
			pkt->seq = htons(pkt->seq);
			// 전송시 시간 계산
			cur_time = time(NULL);
			pkt->time = htonl(cur_time);
			// 전송
			sendto(serv_sock, pkt, sizeof(pkt_s), 0, (struct sockaddr *)&serv_adr, sizeof(serv_adr));

			// 읽기
			str_len = recvfrom(serv_sock, ack, sizeof(pkt_s) + 1024, 0, (struct sockaddr *)&clnt_adr, &clnt_adr_sz);
			// Seq 비교
			pkt->seq = ntohs(pkt->seq);
			ack->seq = ntohs(ack->seq);
			if (pkt->seq == ack->seq)
			{
				// printf("PKT INFO\nTIME: %ld\nSEQ: %d\nMSG: %s\n", pkt->time, pkt->seq, pkt->message);
				seq_flag = 1;
				break;
			}
			if (str_len == -1)
			{
				printf("#%d faile...\n", wait_count);
			}

			wait_count++;
		}
		if (wait_count >= 3)
		{
			printf("-------connection lost!--------\n");
			break;
		}
		// printf("ACK received!\n");
		if (read_cnt < BUF_SIZE)
		{
			printf("File Sent Succesful!\n");
			break;
		}
		pkt->seq++;
	}
	free(pkt);
	free(ack);
	fclose(fp);
	close(serv_sock);
	return 0;
}

void error_handling(char *message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}
