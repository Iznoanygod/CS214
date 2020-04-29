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
#include <math.h>
#include "server.h"


#define BUFF_SIZE 1024

bool running;
TNode *tidHead;
int numClients;


void acceptClients(int port)
{
	int serverFD;
	int opt = 1;
	struct sockaddr_in address;
	int addrlen = sizeof(address);
	
	serverFD = socket(AF_INET, SOCK_STREAM, 0);
	if (serverFD == 0)
	{
		perror("Failed to create server file descriptor.\n");
		exit(EXIT_FAILURE);
	}
	
	int sockopt = setsockopt(serverFD, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt));
	if (sockopt > 0)
	{
		perror("Failed to setsockopt.\n");
		exit(EXIT_FAILURE);
	}
	
	address.sin_family = AF_INET; 
    address.sin_addr.s_addr = INADDR_ANY; 
    address.sin_port = htons(port);

	int bindret = bind(serverFD, (struct sockaddr*)&address, sizeof(address));
	if (bindret < 0)
	{
		perror("Failed to bind to address.\n");
		exit(EXIT_FAILURE);
	}
	
	if (listen(serverFD, 3) < 0) 
    { 
        perror("Failed to listen.\n"); 
        exit(EXIT_FAILURE); 
    } 
	
	while (running)
	{
		int sock = accept(serverFD, (struct sockaddr*) &address, (socklen_t*)&addrlen);
		if (sock < 0)
		{
			perror("Failed to accept new client.\n");
			running = false;
			exit (EXIT_FAILURE);
		}
		
		numClients++;
		TNode *curr = tidHead;
	
		int *args = (int*)malloc(sizeof(int));
		args[0] = sock;
		args[1] = numClients;
		
		pthread_create(&(curr->tid), NULL, handleClient, (void*) args);
		
		TNode *next = malloc(sizeof(TNode));
		curr->next = next;
		curr = next;
	}
	
	
	collectThreads(tidHead);
}

void * handleClient(void * args)
{
	int sock = ((int*) args)[0];
	int clientNum = ((int*) args)[1];
	
	char buffer[BUFF_SIZE] = {0};
	int readret = read(sock, buffer, BUFF_SIZE);
	
	printf("%s\n", buffer);
	
	char message[20] = "Welcome client ";
	strcat(message, itoa(clientNum));
	
	printf("here");
	send(sock, message, strlen(message), 0);
	
	close(sock);
}

char * itoa(int i) 
{
	printf("here");
	int size = (int)((ceil(log10(i))+1)*sizeof(char));
	char * str = (char *) malloc(size);
	sprintf(str, "%i", i);
	return str;
}

void collectThreads(TNode *head)
{
	TNode *curr = head;
	while (curr != NULL)
	{
		pthread_join(curr->tid, NULL);
		TNode *temp = curr;
		curr = curr->next;
		free(temp);
	}
}
	
void sig_handler(int sig)
{
	printf("\nStopping server...\n");
	running = false;
	collectThreads(tidHead);
	exit(0);
}

int main(int argc, char *argv[])
{
	signal(SIGINT, sig_handler);
	
	tidHead = malloc(sizeof(TNode));
	numClients = 0;
	running = true;
	
	acceptClients(25585);
}







	