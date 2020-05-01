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
#include <errno.h>
#include <math.h>
#include <libtar.h>
#include <sys/types.h>
#include <dirent.h>
#include "server.h"


#define BUFF_SIZE 1024

bool running;
TNode *tidHead;
int numClients;
PNode *projects;
pthread_mutex_t pLock;

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
	
	char message[24] = "20:clientConnectSuccess";
	send(sock, message, strlen(message), 0);
    int i;
    for(i = 0; ;i++){
        int in = read(sock, buffer+i, 1);
        if(in < 1){
            printf("Error: failed reading message from client, closing socket\n");
            send(sock, "11:messageFail", 15, 0);
            close(sock);
            return NULL;
        }
        if(buffer[i] == ':'){
            buffer[i] = '\0';
            break;
        }
    }
    if(!strcmp(buffer, "create")){
        int length;
        int i;
        for(int i = 0; ;i++){
            int in = read(sock, buffer+1, 1);
            if(in < 1){
                printf("Error: failed reading message from client, closing socket\n");
                send(sock, "11:messageFail", 15, 0);
                close(sock);
                return NULL;
            }
            if(buffer[i] == ':'){
                buffer[i] = '\0';
                break;
            }
        }
        length = atoi(buffer);
        int in = read(sock, buffer, length);
        if(in < length){
            printf("Error: failed reading message from client, closing socket\n");
            send(sock, "11:messageFail", 15, 0);
            close(sock);
            return NULL;
        }

        buffer[length] = '\0';
        int projCr = createProject(buffer);
        if(projCr == 0){
            printf("Error: failed to create project, project already exists\n");
            send(sock, "19:projectAlreadyExist", 23, 0);
            close(sock);
            return NULL;
        }
        if(projCr == -1){
            printf("Error: failed to create project directory\n");
            send(sock, "12:projectError", 15, 0);
            close(sock);
            return NULL;
        }
        printf("Project successfully created\n");
    }
    close(sock);
    return NULL;
}

int createProject(char* projName){
    DIR* dir = opendir("projName");
    if(dir){
        closedir(dir);
        return 0;
    }
    else if (ENOENT == errno) {
        if(mkdir(projName, 0777)){
            return -1;
        }
        char path[BUFF_SIZE] = {0};
        strcpy(path, projName);
        strcat(path, "/ver0");
        if(mkdir(path, 0777)){
            return -1;
        }
        strcat(path, "/.Manifest");
        int fd = open(path, O_RDWR | O_CREAT, S_IRWXU);
        write(fd, "0\n", 2);
        close(fd);
        strcpy(path, projName);
        strcat(path, "/.versions");
        fd = open(path, O_RDWR | O_CREAT, S_IRWXU);
        char init[] = "ver0\n";
        write(fd, init, strlen(init));
        close(fd);
        
        //adding mutex stuff
        PNode * temp = malloc(sizeof(PNode));
        temp->project = malloc(strlen(projName) + 1);
        strcpy(temp->project, projName);
        temp->lock = malloc(sizeof(pthread_mutex_t));
        pthread_mutex_init(temp->lock, NULL);
        pthread_mutex_lock(pLock);
        temp->next = projects;
        projects = temp;
        pthread_mutex_unlock(pLock);

        return 1;
    }
    else {
        return -1;
    }
}

char * itoa(int i) 
{
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
	
	int pfd = open(".projects", O_RDWR | O_CREAT, S_IRWXU);
    int fsize = lseek(pfd, 0, SEEK_END);
    lseek(pfd, 0, SEEK_SET);
    char* fileIn = malloc(fsize);
    int readin = 0;
    while(1){
        int status = read(pfd, fileIn + readin, fsize - readin);
        readin += status;
        if(readin == fsize)
            break;
    }
    close(pfd);
    if(fsize){
        char* line = malloc(fsize);
        line[0] = '\0';
        int i;
        for(i = 0; i < fsize; i++){
            if(fileIn[i] = '\n'){
                PNode* temp = malloc(sizeof(PNode));
                temp->project = malloc(strlen(line) + 1);
                strcpy(temp->project, line);
                temp->lock = malloc(sizeof(pthread_mutex_t));
                pthread_mutex_init(temp->lock, NULL);
                temp->next = projects;
                projects = temp;

                line[0] = '\0';
            }
            else{
                strncat(line, fileIn + i, 1);
            }
        }
        free(line);
    }
    free(fileIn);
    tidHead = NULL;
	numClients = 0;
	running = true;
	pLock = malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(pLock, NULL);
	acceptClients(25585);
}







	
