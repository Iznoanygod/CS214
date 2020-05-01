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
#include <zlib.h>
#include "simpleIO.h"
#include "server.h"


#define BUFF_SIZE 1024

int running;
TNode *tidHead;
int numClients;
PNode *projects;
pthread_mutex_t* pLock;

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
	printf("Waiting for clients...\n");
	while (running)
	{
		int sock = accept(serverFD, (struct sockaddr*) &address, (socklen_t*)&addrlen);
		if (sock < 0)
		{
			perror("Failed to accept new client.\n");
			running = 0;
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
        length = atoi(buffer);
        int in = simpleRead(sock, buffer, length);
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
            send(sock, "12:projectError", 16, 0);
            close(sock);
            return NULL;
        }
        printf("Project successfully created\n");
        send(sock, "15:projectSuccess", 19, 0);
        close(sock);
        return NULL;
    }
    if(!strcmp(buffer, "destroy")){
        int length;
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
        length = atoi(buffer);
        int in = simpleRead(sock, buffer, length);
        if(in < length){
            printf("Error: failed reading message from client, closing socker\n");
            send(sock, "11:messageFail", 15, 0);
            close(sock);
            return NULL;
        }

        buffer[length] = '\0';
        int projCr = destroyProject(buffer);
        if(projCr == 0){
            printf("Error: failed to destroy project, project doesn't exist\n");
            send(sock, "15:projectNotExist", 19, 0);
            close(sock);
            return NULL;
        }
        if(projCr == -1){
            printf("Error: failed to destroy project\n");
            send(sock, "12:projectError", 16, 0);
            close(sock);
            return NULL;
        }
        printf("Project successfully deleted\n");
        send(sock, "15:projectSuccess", 19, 0);
        close(sock);
        return NULL;
    }
    close(sock);
    return NULL;
}

int destroyProject(char* projName){
    pthread_mutex_lock(pLock);
    PNode * proj = projects;
    PNode * trail = NULL;
    while(proj != NULL){
        if(!strcmp(projName, proj->project)){
            pthread_mutex_lock(proj->lock);
            free(proj->project);
            if(trail == NULL)
                projects = projects->next;
            else
                trail->next = proj->next;
            
            pthread_mutex_unlock(proj->lock);
            pthread_mutex_destroy(proj->lock);
            
            free(proj->lock);
            free(proj);
            pthread_mutex_unlock(pLock);
            char* path = malloc(strlen(projName) + 8);
            strcpy(path, "rm -rf ");
            strcat(path, projName);
            int state = system(path);
            free(path);
            if(state == -1)
                return -1;
            return 1;
        }
        trail = proj;
        proj = proj->next;
    }
    pthread_mutex_unlock(pLock);
    return 0;
}

int rolebackProject(char* projName, int version){
    pthread_mutex_lock(pLock);
    PNode* proj = projects;
    while(proj != NULL){
        if(!strcmp(proj->project, projName))
            break;
        proj = proj->next;
    }
    pthread_mutex_unlock(pLock);
    if(proj == NULL)
        return -1;
    pthread_mutex_lock(proj->lock);
    char path[BUFF_SIZE] = {0};
    strcpy(path, projName);
    strcat(path, "/.Current");
    int verFD = open(path, O_RDWR);
    char in[BUFF_SIZE] = {0};
    simpleRead(verFD, in, -1);
    close(verFD);
    int currentVer;
    sscanf(in, "ver%d", &currentVer);
    if(currentVer < version){
        pthread_mutex_unlock(proj->lock);
        return 0;
    }
    if(currentVer == version){
        pthread_mutex_unlock(proj->lock);
        return 1;
    }
    verFD = open(path, O_RDWR | O_TRUNC);
    char versionOut[BUFF_SIZE];
    sprintf(versionOut, "ver%d\n", version);
    simpleWrite(verFD, versionOut,  -1);
    close(verFD);
    sprintf(path, "rm -rf %s/ver%d", projName, currentVer);
    system(path);
    int i;
    for(i = version + 1; i < currentVer; i++){
        sprintf(path, "rm %s/ver%d.tar.gz", projName, i);
        system(path);
    }
    //char decompress[BUFF_SIZE];
    sprintf(path, "%s/ver%d.tar.gz", projName, version);
    unTar(path, projName);
    /*gzFile fi = gzopen(path,"rb");
    gzrewind(fi);
    sprintf(path, "%s/ver%d.tar", projName, version);
    int tfd = open(path, O_RDWR | O_CREAT | O_APPEND, S_IRWXU);
    while(!gzeof(fi)){
        int zlen = gzread(fi, decompress, sizeof(decompress));
        write(tfd, decompress, zlen); 
    }
    gzclose(fi);
    close(tfd);
    TAR *pTar = NULL;
    tar_open(&pTar, path, NULL, O_RDONLY, 0777, TAR_GNU);
    tar_extract_all(pTar, projName);
    //deal with decompressed data
    tar_close(pTar);
    remove(path);*/
    
    sprintf(path, "%s/ver%d.tar.gz", projName, version);
    remove(path);
    pthread_mutex_unlock(proj->lock);
    return 1;
}

int currentVersion(char* projName){
    pthread_mutex_lock(pLock);
    PNode* proj = projects;
    while(proj != NULL){
        if(!strcmp(proj->project, projName))
            break;
        proj = proj->next;
    }
    pthread_mutex_unlock(pLock);
    if(proj == NULL)
        return -1;
    pthread_mutex_lock(proj->lock);
    char path[BUFF_SIZE] = {0};
    strcpy(path, projName);
    strcat(path, "/.Current");
    int verFD = open(path, O_RDONLY);
    int verSize = lseek(verFD, 0, SEEK_END);
    lseek(verFD, 0, SEEK_SET);
    char in[BUFF_SIZE] = {0};
    int readin = 0;
    while(1){
        int status = read(verFD,in+readin, verSize-readin);
        readin += status;
        if(readin == verSize)
            break;
    }
    close(verFD);
    in[readin] = '\0';
    int currentVer;
    sscanf(in, "ver%d", &currentVer);
    pthread_mutex_unlock(proj->lock);
    return currentVer;
}

int createProject(char* projName){
    DIR* dir = opendir(projName);
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
        strcat(path, "/.Current");
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
	int size = (ceil(log10(i+1))+1);
    if(i == 0)
        size = 2;
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
	running = 0;
    createProjectFile();
    pthread_mutex_destroy(pLock);
    free(pLock);
    collectThreads(tidHead);
    exit(0);
}

int main(int argc, char *argv[])
{
	signal(SIGINT, sig_handler);
	
	int pfd = open(".projects", O_RDWR | O_CREAT, S_IRWXU);
    char fileIn[BUFF_SIZE] = {0};
    simpleRead(pfd, fileIn, -1);
    close(pfd);
    int fsize = strlen(fileIn);
    if(fsize > 1){
        char* line = malloc(fsize);
        line[0] = '\0';
        int i;
        for(i = 0; i < fsize; i++){
            if(fileIn[i] == '\n'){
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
    tidHead = NULL;
	numClients = 0;
	running = 1;
	pLock = malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(pLock, NULL);
    
    rolebackProject("proj", 0);

    acceptClients(25585);
}
void createProjectFile(){
    int fd = open(".projects", O_RDWR | O_TRUNC | O_CREAT, S_IRWXU);
    pthread_mutex_lock(pLock);
    PNode* temp = projects;
    temp = projects;
    if(temp == NULL)
        simpleWrite(fd, "\n", -1);
    while(temp != NULL){
        simpleWrite(fd, temp->project, -1);
        simpleWrite(fd, "\n", -1);
        temp = temp->next;
    }
    close(fd);
    pthread_mutex_unlock(pLock);
}
