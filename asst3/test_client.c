#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <ctype.h>
#include <pthread.h>
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <signal.h>

#define BUFF_SIZE 1024

int sock;

int getSocket(char *ip, int port)
{
	struct sockaddr_in serv_addr;
	
	int sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0)
	{
		printf("Could not create socket.\n");
		exit(EXIT_FAILURE);
	}
	serv_addr.sin_family = AF_INET; 
    serv_addr.sin_port = htons(port);

	if (inet_pton(AF_INET, ip, &serv_addr.sin_addr) <= 0)
	{
		printf("Invalid address.\n");
		exit(EXIT_FAILURE);
	}
	
	if (connect(sock, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0)
	{
		printf("Failed to connect.");
		exit(EXIT_FAILURE);
	}
	
	return sock;
}


void sig_handler(int sig)
{
	printf("\nStopping client...\n");
	close(sock);
	exit(0);
}

int main(int argc, char *argv[])
{
	signal(SIGINT, sig_handler);
	
	sock = getSocket("127.0.0.1", 25585);
	send(sock, "hey", strlen("hey"), 0);
	
	char buff[BUFF_SIZE] = {0};
	int totalBytesRead = 0;
	while (1)
	{
		int bytesRead = read(sock, buff+totalBytesRead, BUFF_SIZE);
		totalBytesRead+=bytesRead;
		
		if (strcmp(buff,"") != 0)
		{
			printf("%s\n", buff);
		}
		
		memset(buff, 0, BUFF_SIZE);
	}
	
}