#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <time.h>

#define BUF_SIZE 30
void error_handling(char *message);

typedef struct 
{
	long time;
	int seq;
	char message[BUF_SIZE];
} pkt_c;

int main(int argc, char *argv[])
{
	int sock;
	int str_len;
	int total_len = 0;
	socklen_t from_adr_sz;
	int first_flag = 0;
	long first_time;

	int file_size;
	int total_size = 0;

	FILE *fp;
	char file_name[30];

	long total_time;
	long throughput;

	pkt_c * pkt = malloc(sizeof(pkt_c));
	pkt_c * ack = malloc(sizeof(pkt_c));
	time_t cur_time;
	
	struct sockaddr_in serv_adr, from_adr;
	if (argc != 2) {
		printf("Usage : %s <port>\n", argv[0]);
		exit(1);
	}
	
	sock = socket(PF_INET, SOCK_DGRAM, 0);   
	if (sock == -1)
		error_handling("socket() error");
	
	memset(&serv_adr, 0, sizeof(serv_adr));
	serv_adr.sin_family = AF_INET;
	serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_adr.sin_port = htons(atoi(argv[1]));
	
	if (bind(sock, (struct sockaddr*)&serv_adr, sizeof(serv_adr)) == -1)
		error_handling("bind() error");
	
	printf("Listening.....\n");
	while (1)
	{	
		from_adr_sz = sizeof(from_adr);
		str_len = recvfrom(sock, pkt, sizeof(pkt_c), 0, (struct sockaddr*)&from_adr, &from_adr_sz);
		cur_time = time(NULL);
		if(first_flag == 0)
		{
			file_size = atoi(pkt->message);
			first_time = ntohl(pkt->time);
			printf("file size: %d\n", file_size);
			first_flag++;
		}
		else if(first_flag == 1)
		{
			strcpy(file_name, pkt->message);
			printf("Downloading file [%s]\n", pkt->message);
			fp = fopen(file_name, "wb");
			// file_size += strlen(pkt->message)+1;
			first_flag++;
		}
		else
		{
			total_size += strlen(pkt->message);
			fwrite((void*)pkt->message, 1, strlen(pkt->message), fp);
		}
		total_len += str_len;

		ack->seq = ntohs(pkt->seq);
		pkt->time = ntohl(pkt->time);
		
		ack->seq = htons(ack->seq);
		strcpy(ack->message, "Thank you!");
		ack->time = htonl(cur_time);
		sendto(sock, ack, sizeof(pkt_c), 0, (struct sockaddr*)&from_adr, from_adr_sz);

		if(file_size <= total_size)
			break;
	}	
	fclose(fp);
	total_time = (cur_time == first_time) ? 1 : (cur_time - first_time);
	printf("Download-start-time: %ld\t Current-time: %ld\n", first_time, cur_time);
	printf("Total size: %d\n", total_size);
	printf("Time taken: %ld\n", total_time);
	throughput = (total_len*8)/total_time;

	printf("\nThroughput: %ld bps\n", throughput);


	free(pkt);
	free(ack);
	close(sock);
	return 0;
}

void error_handling(char *message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}