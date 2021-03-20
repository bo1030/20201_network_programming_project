#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/select.h>
#define BUF_SIZE 1024

void error_handling(char *message);

int main(int argc, char * argv[])
{
	FILE * fp;
	int sock, pid;
	struct sockaddr_in serv_adr;
	char message[BUF_SIZE], temp[BUF_SIZE];
	int str_len, result, read_cnt;
	struct timeval timeout;
	fd_set reads, temps;
	if(argc != 4)
	{
		printf("Usage: %s <IP> <PORT> <USER NAME>\n", argv[0]);
		exit(1);
	}
	
	sock = socket(PF_INET, SOCK_STREAM, 0);
	if(sock == -1)
		error_handling("socket() error");
	memset(&serv_adr, 0, sizeof(serv_adr));
	serv_adr.sin_family = AF_INET;
	serv_adr.sin_addr.s_addr = inet_addr(argv[1]);
	serv_adr.sin_port = htons(atoi(argv[2]));
	
	if(connect(sock, (struct sockaddr*)&serv_adr, sizeof(serv_adr)) == -1)
		error_handling("connect() error");
	FD_ZERO(&reads);
	FD_SET(0, &reads);
	FD_SET(sock, &reads);
	write(sock, argv[3], strlen(argv[3]));
	while(1)
	{
		temps = reads;
		timeout.tv_sec = 1;
		timeout.tv_usec = 0;
		result = select(sock+1, &temps, 0, 0, &timeout);
		if(result == -1)
		{
			puts("select() error!");
			break;
		}
		else if(result == 0)
		{
			continue;
		}
		
		if(FD_ISSET(0, &temps))
		{
			str_len = read(0, temp, BUF_SIZE);
			temp[str_len] = 0;
				
			if(!strcmp(temp, "q\n") || !strcmp(temp, "Q\n"))
				break;
			if(!strcmp(temp, "file\n"))
			{
				str_len = read(0, temp, BUF_SIZE);
				temp[str_len-1] = 0;
				write(sock, "file\n", strlen("file\n"));
				pid = fork();
				if(pid==0)
				{
				    int serv_sd, clnt_sd;
				    char buf[BUF_SIZE];
				    int read_cnt;
				    struct sockaddr_in adr, clnt_adr;
				    socklen_t clnt_adr_sz;
				    fp = fopen(temp, "rb");
				    serv_sd = socket(PF_INET, SOCK_STREAM, 0);
				    if(serv_sd == -1)
					error_handling("socket() error");
				    
				    memset(&serv_adr, 0, sizeof(serv_adr));
				    adr.sin_family = AF_INET;
				    adr.sin_addr.s_addr = htonl(INADDR_ANY);
				    adr.sin_port = htons(atoi("9002"));
				    if(bind(serv_sd, (struct sockaddr*)&adr, sizeof(adr)) == -1)
					error_handling("bind() error");
				    if(listen(serv_sd, 5) == -1)
					error_handling("listen() error");
				    clnt_adr_sz = sizeof(clnt_adr);
				    clnt_sd = accept(serv_sd, (struct sockaddr*)&clnt_adr, &clnt_adr_sz);
				    while(1)
				    {
					read_cnt = fread((void*)buf, 1, BUF_SIZE, fp);
					if(read_cnt < BUF_SIZE)
					{
					    write(clnt_sd, buf, read_cnt);
					    break;
					}
					write(clnt_sd, buf, BUF_SIZE);
				    }
				    shutdown(clnt_sd, SHUT_WR);
				    fclose(fp);
				    close(clnt_sd);
				    close(serv_sd);
				    exit(0);
				}
			}
			else
			{
				sprintf(message, "%s:%s\n", argv[3], temp);
				write(sock, message, strlen(message));
			}
		}
		else
		{
			str_len = read(sock, message, BUF_SIZE-1);
			message[str_len] = 0;
			if(!strcmp(message, "file"))
			{
				pid = fork();
				if(pid==0)
				{
				    int sd;
				    char buf[BUF_SIZE];
				    int read_cnt;
				    struct sockaddr_in adr;
				    socklen_t clnt_adr_sz;
				    fp = fopen("receive.dat", "wb");
				    sd = socket(PF_INET, SOCK_STREAM, 0);
				    
				    memset(&serv_adr, 0, sizeof(serv_adr));
				    adr.sin_family = AF_INET;
				    adr.sin_addr.s_addr = inet_addr("127.0.0.1");
				    adr.sin_port = htons(atoi("9002"));

				    connect(sd, (struct sockaddr*)&adr, sizeof(adr));
				    
				    while((read_cnt = read(sd, buf, BUF_SIZE)) != 0)
					fwrite((void*)buf, 1, read_cnt, fp);
				    fclose(fp);
				    close(sd);
				    exit(0);
				}
			}
			printf("%s", message);
		}
	}
	close(sock);
	return 0;
}

void error_handling(char *message)
{
        fputs(message, stderr);
        fputc('\n', stderr);
        exit(1);
}
