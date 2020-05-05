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
//#include <libtar.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
//#include <zlib.h>
#include "simpleIO.h"
#include "server.h"

#define BUFF_SIZE 4096

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
		TNode *curr = malloc(sizeof(TNode));
        curr->tid = 0;
        curr->next = tidHead;
        tidHead = curr;
		int *args = malloc(sizeof(int) * 2);
		args[0] = sock;
		args[1] = numClients;
		
		pthread_create(&(curr->tid), NULL, handleClient, (void*) args);
	}
	
	collectThreads(tidHead);
}

void * handleClient(void * args)
{
	int sock = ((int*) args)[0];
	int clientNum = ((int*) args)[1];
	free(args);
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
    if(!strcmp(buffer, "update")){
        char inbound[BUFF_SIZE] = {0};
        char outbound[BUFF_SIZE] = {0};
        char systemC[BUFF_SIZE] = {0};
        int i;
        for(i = 0; ; i++){
            read(sock, inbound + i, 1);
            if(inbound[i] == ':')
                break;
        }
        inbound[i] = '\0';
        int len = atoi(inbound);
        simpleRead(sock, inbound, len);
        char* projName = malloc(len+1);
        strcpy(projName, inbound);
        pthread_mutex_lock(pLock);
        PNode* pr = projects;
        while(pr != NULL){
            if(!strcmp(pr->project, projName))
                break;
            pr = pr->next;
        }
        pthread_mutex_unlock(pLock);
        if(pr == NULL){
            send(sock, "0:15:projectNotExist", 20, 0);
            close(sock);
            return NULL;
        }
        sprintf(systemC, "%s/.Current", projName);
        int cfd = open(systemC, O_RDONLY);
        simpleRead(cfd, inbound, -1);
        close(cfd);
        char currentVersion[BUFF_SIZE] = {0};
        sscanf(inbound, "%s", currentVersion);
        sprintf(systemC, "gzip -c %s/%s/.Manifest > %s/%s/.Manifest.gz", projName, currentVersion, projName, currentVersion);
        system(systemC);
        sprintf(systemC, "%s/%s/.Manifest.gz", projName, currentVersion);
        int mfd = open(systemC, O_RDONLY);
        int msize = lseek(mfd, 0, SEEK_END);
        lseek(mfd, 0, SEEK_SET);
        char* manifestgz = malloc(msize+1);
        simpleRead(mfd, manifestgz, msize);
        close(mfd);
        sprintf(outbound, "%d:", msize);
        send(sock, outbound, strlen(outbound), 0);
        send(sock, manifestgz, msize, 0);
        remove(systemC);
        pthread_mutex_unlock(pLock);
        close(sock);
    }
    if(!strcmp(buffer, "upgrade")){
        char outbound[BUFF_SIZE] = {0};
        char inbound[BUFF_SIZE] = {0};
        char systemC[BUFF_SIZE] = {0};
        int i;
        for(i = 0; ; i++){
            read(sock, inbound + i, 1);
            if(inbound[i] == ':')
                break;
        }
        inbound[i] = '\0';
        int len = atoi(inbound);
        simpleRead(sock, inbound, len);
        char* projName = malloc(len+1);
        strcpy(projName, inbound);
        pthread_mutex_lock(pLock);
        PNode* pr = projects;
        while(strcmp(pr->project, projName)){
            pr = pr->next;
        }
        pthread_mutex_unlock(pLock);
        pthread_mutex_lock(pr->lock);
        send(sock, "8:continue", 10, 0);
        for(i = 0; ; i++){
            read(sock, inbound + i, 1);
            if(inbound[i] == ':')
                break;
        }
        inbound[i] = '\0';
        int uSize = atoi(inbound);
        simpleRead(sock, inbound, uSize);
        sprintf(systemC, "%s/.Update.gz", projName);
        int ufd = open(systemC, O_CREAT | O_RDWR, S_IRWXU);
        simpleWrite(ufd, inbound, uSize);
        close(ufd);
        sprintf(systemC, "gunzip %s/.Update.gz", projName);
        system(systemC);
        sprintf(systemC, "%s/.Changes", projName);
        mkdir(systemC, 0777);
        sprintf(systemC, "%s/.Update", projName);
        ufd = open(systemC, O_RDWR);
        uSize = lseek(ufd, 0, SEEK_END);
        lseek(ufd, 0, SEEK_SET);
        char* update = malloc(uSize + 1);
        simpleRead(ufd, update, uSize);
        close(ufd);
        sprintf(systemC, "%s/.Current", projName);
        int curFD = open(systemC, O_RDWR);
        for(i = 0; ; i++){
            read(curFD, inbound+i, 1);
            if(inbound[i] == '\n')
                break;
        }
        inbound[i] = '\0';
        close(curFD);
        char* current = malloc(strlen(inbound)+1);
        strcpy(current, inbound);
        char line[BUFF_SIZE] = {0};
        sprintf(systemC, "cp %s/%s/.Manifest %s/.Changes", projName, current, projName);
        system(systemC);
        for(i = 0; i < uSize; i++){
            if(update[i] == '\n'){
                char action;
                char* path = malloc(strlen(line));
                sscanf(line, "%c %s", &action, path);
                if(action == 'A' || action == 'M'){
                    sprintf(systemC, "cd %s/%s; cp --parents %s ../.Changes", projName, current, path);
                    system(systemC);
                }
            }
            else
                strncat(line, update+i, 1);
        }
        sprintf(systemC, "cd %s; tar czf .Changes.tar.gz .Changes;rm -rf .Changes; rm .Update", projName);
        system(systemC);
        sprintf(systemC,"%s/.Changes.tar.gz", projName);
        int chgzfd = open(systemC, O_RDWR);
        int csize = lseek(chgzfd, 0, SEEK_END);
        lseek(chgzfd, 0, SEEK_SET);
        char* chgz = malloc(csize+1);
        simpleRead(chgzfd, chgz, csize);
        close(chgzfd);
        remove(systemC);
        sprintf(outbound, "%d:", csize);
        send(sock, outbound, strlen(outbound), 0);
        send(sock, chgz, csize, 0);
        free(projName);
        close(sock);
        pthread_mutex_unlock(pr->lock);
    }
    if(!strcmp(buffer, "commit")){
        char inbound[BUFF_SIZE] = {0};
        char outbound[BUFF_SIZE] = {0};
        char systemC[BUFF_SIZE] = {0};
        int i;
        for(i = 0; ; i++){
            read(sock, inbound + i, 1);
            if(inbound[i] == ':')
                break;
        }
        inbound[i] = '\0';
        int len = atoi(inbound);
        simpleRead(sock, inbound, len);
        char* projName = malloc(len+1);
        strcpy(projName, inbound);
        pthread_mutex_lock(pLock);
        PNode* pr = projects;
        while(pr != NULL){
            if(!strcmp(pr->project, projName))
                break;
            pr = pr->next;
        }
        pthread_mutex_unlock(pLock);
        if(pr == NULL){
            send(sock, "0:15:projectNotExist", 20, 0);
            close(sock);
            return NULL;
        }
        pthread_mutex_lock(pr->lock);
        sprintf(systemC, "%s/.Current", projName);
        int cfd = open(systemC, O_RDONLY);
        simpleRead(cfd, inbound, -1);
        close(cfd);
        char currentVersion[BUFF_SIZE] = {0};
        sscanf(inbound, "%s", currentVersion);
        sprintf(systemC, "gzip -c %s/%s/.Manifest > %s/%s/.Manifest.gz", projName, currentVersion, projName, currentVersion);
        system(systemC);
        sprintf(systemC, "%s/%s/.Manifest.gz", projName, currentVersion);
        int mfd = open(systemC, O_RDONLY);
        int msize = lseek(mfd, 0, SEEK_END);
        lseek(mfd, 0, SEEK_SET);
        char* manifestgz = malloc(msize+1);
        simpleRead(mfd, manifestgz, msize);
        close(mfd);
        sprintf(outbound, "%d:", msize);
        send(sock, outbound, strlen(outbound), 0);
        send(sock, manifestgz, msize, 0);
        remove(systemC);
        for(i = 0; ; i++){
            read(sock, inbound + i, 1);
            if(inbound[i] == ':')
                break;
        }
        inbound[i] = '\0';
        len = atoi(inbound);
        simpleRead(sock, inbound, len);
        if(!strcmp(inbound, "commitSuccess")){
            for(i = 0; ; i++){
                read(sock, inbound + i, 1);
                if(inbound[i] == ':')
                    break;
            }
            inbound[i] = '\0';
            len = atoi(inbound);
            char* newCommit = malloc(len+1);
            simpleRead(sock, newCommit, len);
            sprintf(systemC, "%s/.tCommit.gz", projName);
            int scfd = open(systemC, O_CREAT | O_RDWR, S_IRWXU);
            simpleWrite(scfd, newCommit, len);
            close(scfd);
            sprintf(systemC, "gunzip %s/.tCommit.gz", projName);
            system(systemC);
            sprintf(systemC, "%s/.tCommit", projName);
            char* sum = md5(systemC);
            sprintf(systemC, "mv %s/.tCommit %s/commits/%s", projName, projName, sum);
            system(systemC);
            free(sum);
            send(sock, "7:success", 9, 0);
            free(newCommit);
        }
        pthread_mutex_unlock(pr->lock);
        close(mfd);
        free(projName);
        free(manifestgz);
        close(sock);
    }
    if(!strcmp(buffer, "push")){
        char systemC[BUFF_SIZE] = {0};
        char inbound[BUFF_SIZE] = {0};
        char outbound[BUFF_SIZE] = {0};
        
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
        char* projN = malloc(length + 1);
        int in = simpleRead(sock, projN, length);
        if(in <= 1){
            printf("Error: failed reading message from client, closing socket\n");
            send(sock, "11:messageFail", 15, 0);
            close(sock);
            return NULL;
        }
        pthread_mutex_lock(pLock);
        PNode* pr = projects;
        while(pr != NULL){
            if(!strcmp(pr->project, projN))
                break;
            pr = pr->next;
        }
        pthread_mutex_unlock(pLock);
        if(pr == NULL){
            send(sock, "15:projectNotExist", 19, 0);
            close(sock);
            return NULL;
        }
        send(sock, "8:continue", 10, 0);
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
        pthread_mutex_lock(pr->lock);
        int gzlength = atoi(buffer);
        sprintf(systemC, "%s/.Commit.gz", projN);
        int cgz = open(systemC, O_CREAT | O_RDWR, S_IRWXU);
        char* comgz = malloc(gzlength + 1);
        simpleRead(sock, comgz, gzlength);
        simpleWrite(cgz, comgz, gzlength);
        close(cgz);
        free(comgz);
        
        sprintf(systemC, "gunzip %s/.Commit.gz", projN);
        system(systemC);
        sprintf(systemC, "%s/.Commit", projN);
        char* md5sum = md5(systemC);
        sprintf(systemC, "%s/commits/%s", projN, md5sum);
        free(md5sum);
        if(access(systemC, F_OK) == -1){
            send(sock, "8:pushFail", 10, 0);
            close(sock);
            sprintf(systemC, "%s/.Commit", projN);
            remove(systemC);
            pthread_mutex_unlock(pr->lock);
            return NULL;
        }
        send(sock, "8:continue", 10, 0);
        sprintf(systemC, "%s/.Current", projN);
        int curFD = open(systemC, O_RDWR);
        char currentFile[BUFF_SIZE];
        simpleRead(curFD, currentFile, -1);
        lseek(curFD, 0, SEEK_SET);
        int version;
        sscanf(currentFile, "ver%d", &version);
        sprintf(systemC, "cp -R %s/ver%d %s/ver%d; tar czf %s/ver%d.tar.gz %s/ver%d; rm -rf %s/ver%d", projN, version, projN, version + 1, projN, version, projN, version, projN, version);
        system(systemC);
        sprintf(systemC, "ver%d\n", version+1);
        simpleWrite(curFD, systemC, -1);
        close(curFD);
        for(i = 0; ; i++){
            read(sock, inbound+i, 1);
            if(inbound[i] == ':')
                break;
        }
        inbound[i] = '\0';
        int changeSize = atoi(inbound);
        char* cFile = malloc(changeSize+1);
        read(sock, cFile, changeSize);
        sprintf(systemC, "%s/.Changes.tar.gz", projN);
        int ChFD = open(systemC, O_RDWR | O_CREAT, S_IRWXU);
        simpleWrite(ChFD, cFile, changeSize);
        close(ChFD);
        sprintf(systemC, "tar xzf %s/.Changes.tar.gz", projN);
        system(systemC);
        sprintf(systemC, "%s/.Changes.tar.gz", projN);
        remove(systemC);
        sprintf(systemC, "%s/.Commit", projN);
        int comFD = open(systemC, O_RDONLY);
        int comSize = lseek(comFD, 0, SEEK_END);
        lseek(comFD, 0, SEEK_SET);
        char* comFile = malloc(comSize+1);
        simpleRead(comFD, comFile, comSize);
        close(comFD);

        sprintf(outbound, "%s/.History", projN);
        int hist = open(outbound, O_RDWR | O_APPEND);
        sprintf(outbound, "Version %d\n", version + 1);
        simpleWrite(hist, outbound, strlen(outbound));
        simpleWrite(hist, comFile, comSize);
        simpleWrite(hist, "\n", 1);
        close(hist);

        char manifestPath[BUFF_SIZE];
        sprintf(manifestPath, "%s/ver%d/.Manifest", projN, version + 1);
        int mfd = open(manifestPath, O_RDWR);
        int msize = lseek(mfd, 0, SEEK_END);
        lseek(mfd, 0, SEEK_SET);
        char* mbuffer = malloc(msize+1);
        simpleRead(mfd, mbuffer, -1);
        char* line = malloc(msize + 1);
        line[0] = '\0';
        for(i = 0; ; i++){
            if(mbuffer[i] == '\n')
                break;
        }
        FNode* fileList = NULL;
        for(++i;i < msize; i++){
            if(mbuffer[i] == '\n'){
                int linesize = strlen(line);
                char* FName = malloc(linesize);
                char* FHash = malloc(linesize);
                int FVersion;
                sscanf(line, "%s %d %s", FName, &FVersion, FHash);
                FNode* temp = malloc(sizeof(FNode));
                temp->path = FName;
                temp->hash = FHash;
                temp->version = FVersion;
                temp->next = fileList;
                fileList = temp;
                line[0] = '\0';
            }
            else{
                strncat(line, mbuffer+i, 1);
            }
        }
        free(line);
        char* cline = malloc(comSize+1);
        version++;
        cline[0] = '\0';
        for(i = 0; i < comSize; i++){
            if(comFile[i] == '\n'){
                char act;
                char* path = malloc(strlen(cline));
                char* hash = malloc(33);
                sscanf(cline, "%c %s %s", &act, path, hash);
                if(act == 'R'){
                    sprintf(systemC, "rm %s/ver%d/%s", projN, version, path);
                    system(systemC);
                    FNode* find = fileList;
                    FNode* trail = NULL;
                    while(strcmp(find->path, path)){
                        trail = find;
                        find = find->next;
                    }
                    if(trail == NULL){
                        fileList = fileList->next;
                        free(find->path);
                        free(find->hash);
                        free(find);
                    }
                    else{
                        trail->next = find->next;
                        free(find->path);
                        free(find->hash);
                        free(find);
                    }
                }
                else{
                    sprintf(systemC, "cd %s/.Changes; cp --parent %s ../ver%d", projN, path, version);
                    system(systemC);
                    if(act == 'A'){
                        FNode* temp = malloc(sizeof(FNode));
                        temp->path = path;
                        temp->hash = hash;
                        temp->next = fileList;
                        temp->version = 0;
                        fileList = temp;
                    }
                    else{
                        FNode* temp = fileList;
                        while(strcmp(temp->path, path)){
                            temp = temp->next;
                        }
                        free(temp->hash);
                        temp->hash = hash;
                        temp->version++;
                    }
                }
                cline[0] = '\0';
            }
            else{
                strncat(cline, comFile+i, 1);
            }
        }
        close(mfd);
        sprintf(systemC, "%s/ver%d/.Manifest", projN, version);
        mfd = open(systemC, O_TRUNC | O_RDWR);
        insertionSort(&fileList);
        sprintf(systemC, "%d\n", version);
        write(mfd, systemC, strlen(systemC));
        while(fileList != NULL){
            FNode* temp = fileList;
            fileList = fileList->next;
            sprintf(systemC, "%s %d %s\n", temp->path, temp->version, temp->hash);
            write(mfd, systemC, strlen(systemC));
            free(temp->path);
            free(temp->hash);
            free(temp);
        }
        close(mfd);
        sprintf(systemC, "rm -rf %s/.Changes; rm %s/.Commit; rm -rf %s/commits; mkdir %s/commits", projN, projN, projN, projN);
        free(mbuffer);
        free(comFile);
        free(projN);
        free(cline);
        free(cFile);
        system(systemC);
        pthread_mutex_unlock(pr->lock);
        close(sock);
    }
    if(!strcmp(buffer, "checkout")){
        int length;
        int i;
        for(i = 0; ;i++){
            int in = read(sock, buffer+i, 1);
            if(in < 1){
                printf("Error: failed reading message from client, closing socket\n");
                send(sock, "0:11:messageFail", 16, 0);
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
        if(in < 1){
            printf("Error: failed reading message from client, closing socket\n");
            send(sock, "0:11:messageFail", 16, 0);
            close(sock);
            return NULL;
        }

        buffer[length] = '\0';
        pthread_mutex_lock(pLock);
        PNode* pr = projects;
        while(pr != NULL){
            if(!strcmp(pr->project, buffer))
                break;
            pr = pr->next;
        }
        pthread_mutex_unlock(pLock);
        if(pr == NULL){
            send(sock, "0:15:projectNotExist", 20, 0);
            close(sock);
            return NULL;
        }
        pthread_mutex_lock(pr->lock);
        char currentVersion[BUFF_SIZE] = {0};
        char* currentFile = malloc(512);
        sprintf(currentFile, "%s/.Current", buffer);
        int cfd = open(currentFile, O_RDONLY);
        simpleRead(cfd, currentVersion, -1);
        currentVersion[strlen(currentVersion) - 1] = '\0';
        free(currentFile);
        close(cfd);
        char* systemCopy = malloc(512);
        sprintf(systemCopy, "cp -R %s/%s %s/%s; cd %s; tar czf %s.tar.gz %s; rm -rf %s; cd ..", buffer, currentVersion, buffer, buffer, buffer, buffer, buffer, buffer);
        system(systemCopy);
        sprintf(systemCopy, "%s/%s.tar.gz", buffer, buffer);
        int tfd = open(systemCopy, O_RDONLY);
        int size = lseek(tfd, 0, SEEK_END);
        char* sC = itoa(size);
        send(sock, sC, strlen(sC), 0);
        send(sock, ":", 1, 0);
        free(sC);
        lseek(tfd, 0, SEEK_SET);
        char* tarBuf = malloc(size+1);
        int rdwr = 0;
        while(1){
            int status = simpleRead(tfd, tarBuf, size);
            send(sock, tarBuf, status, 0);
            rdwr += status;
            if(rdwr == size)
                break;
        }
        remove(systemCopy);
        close(tfd);
        free(tarBuf);
        close(sock);
        free(systemCopy);
        pthread_mutex_unlock(pr->lock);
    }
    if(!strcmp(buffer, "rollback")){
        int length;
        int i;
        for(i = 0; ; i++){
            int in = read(sock, buffer+i, 1);
            if(in < 1){
                printf("Error: failed reading message form cient, closing socket\n");
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
        if(in < 1){
            printf("Error: failed reading message from client, closing socket\n");
            send(sock, "11:messageFail", 15, 0);
            close(sock);
            return NULL;
        }
        buffer[length] = '\0';
        char* projName = malloc(strlen(buffer) + 1);
        strcpy(projName, buffer);
        read(sock, buffer, 1);
        for(i = 0; ; i++){
            int in = read(sock, buffer+i, 1);
            if(in < 1){
                printf("Error: failed reading message form cient, closing socket\n");
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
        in = simpleRead(sock, buffer, length);
        if(in < 1){
            printf("Error: failed reading message from client, closing socket\n");
            send(sock, "11:messageFail", 15, 0);
            close(sock);
            return NULL;
        }
        buffer[length] = '\0';
        if(!isNumber(buffer)){
            send(sock, "14:invalidVersion", 18, 0);
            close(sock);
            free(projName);
            return NULL;
        }
        int version = atoi(buffer);
        int state = rollbackProject(projName, version);
        if(state == -1){
            send(sock, "15:projectNotExist", 19, 0);
        }
        else if(!state){
            send(sock, "14:invalidVersion", 18, 0);
        }
        else{
            send(sock, "15:projectSuccess", 19, 0);
        }
        free(projName);
        close(sock);
        return NULL;
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
        if(in < 1){
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
        send(sock, "14:projectSuccess", 17, 0);
        send(sock, ":", 1, 0);
        char manPath[512] = {0};
        sprintf(manPath, "gzip -c %s/ver0/.Manifest > %s/ver0/.Manifest.gz", buffer, buffer);
        system(manPath);
        sprintf(manPath, "%s/ver0/.Manifest.gz", buffer);
        int mfd = open(manPath, O_RDONLY);
        int msize = lseek(mfd, 0, SEEK_END);
        char* msizec = itoa(msize);
        send(sock, msizec, strlen(msizec), 0);
        send(sock, ":", 1, 0);
        free(msizec);
        lseek(mfd, 0, SEEK_SET);
        char* manifestc = malloc(msize+1);
        simpleRead(mfd, manifestc, msize);
        close(mfd);
        remove (manPath);
        send(sock, manifestc, msize, 0);
        free(manifestc);
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
        if(in < 1){
            printf("Error: failed reading message from client, closing socket\n");
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
    if(!strcmp(buffer, "currentVersion")){
        int length;
        int i;
        for(i = 0; ;i++){
            int in = read(sock, buffer+i, 1);
            if(in < 1){
                printf("Error: failed reading message from client, closing socket\n");
                send(sock, "0:11:messageFail", 16, 0);
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
        if(in <= 1){
            printf("Error: failed reading message from client, closing socket\n");
            send(sock, "0:11:messageFail", 16, 0);
            close(sock);
            return NULL;
        }
        
        buffer[length] = '\0';
        PNode* proj = projects;
        while(proj != NULL){
            if(!strcmp(proj->project, buffer))
                break;
            proj = proj->next;
        }
        if(proj == NULL){
            printf("Error: failed to get current version, project doesn't exist\n");
            send(sock, "0:15:projectNotExist", 20, 0);
            close(sock);
            return NULL;
        }
        pthread_mutex_lock(proj->lock);
        char* projCur = malloc(strlen(buffer) + 10);
        sprintf(projCur, "%s/.Current", buffer);
        int curFD = open(projCur, O_RDONLY);
        free(projCur);
        char version[BUFF_SIZE] = {0};
        simpleRead(curFD, version, -1);
        version[strlen(version) - 1] = '\0';
        close(curFD);
        char* sysManPathgz = malloc(strlen(buffer) + strlen(version) + 20);
        char* ManPathgz = malloc(strlen(buffer) + strlen(version) + 15);
        sprintf(sysManPathgz, "gzip -c %s/%s/.Manifest > %s/%s/.Manifest.gz", buffer, version, buffer, version);
        sprintf(ManPathgz, "%s/%s/.Manifest.gz", buffer, version);
        system(sysManPathgz);
        int gfz = open(ManPathgz, O_RDONLY);
        int gzs = lseek(gfz, 0, SEEK_END);
        lseek(gfz, 0, SEEK_SET);
        char* gzsC = itoa(gzs);
        send(sock, gzsC, strlen(gzsC), 0);
        free(gzsC);
        send(sock, ":", 1, 0);
        int rdwr = 0;
        char gzin[BUFF_SIZE] = {0};
        while(1){
            int status = simpleRead(gfz, gzin, 1024);
            send(sock, gzin, status, 0);
            rdwr += status;
            if(rdwr == gzs)
                break;
        }
        remove(ManPathgz);
        free(sysManPathgz);
        free(ManPathgz);
        close(gfz);
        close(sock);
        pthread_mutex_unlock(proj->lock);
        return NULL;
    }
    if(!strcmp(buffer, "history")){
        int length;
        int i;
        for(i = 0; ;i++){
            int in = read(sock, buffer+i, 1);
            if(in < 1){
                printf("Error: failed reading message from client, closing socket\n");
                send(sock, "0:11:messageFail", 16, 0);
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
        if(in <= 1){
            printf("Error: failed reading message from client, closing socket\n");
            send(sock, "0:11:messageFail", 16, 0);
            close(sock);
            return NULL;
        }

        buffer[length] = '\0';
        PNode* proj = projects;
        while(proj != NULL){
            if(!strcmp(proj->project, buffer))
                break;
            proj = proj->next;
        }
        if(proj == NULL){
            printf("Error: failed to get current version, project doesn't exist\n");
            send(sock, "0:15:projectNotExist", 20, 0);
            close(sock);
            return NULL;
        }
        pthread_mutex_lock(proj->lock);
        char* sysManPathgz = malloc(strlen(buffer) + 20);
        char* ManPathgz = malloc(strlen(buffer) + 15);
        sprintf(sysManPathgz, "gzip -c %s/.History > %s/.History", buffer, buffer);
        sprintf(ManPathgz, "%s/.History.gz", buffer);
        system(sysManPathgz);
        int gfz = open(ManPathgz, O_RDONLY);
        int gzs = lseek(gfz, 0, SEEK_END);
        lseek(gfz, 0, SEEK_SET);
        char* gzsC = itoa(gzs);
        send(sock, gzsC, strlen(gzsC), 0);
        free(gzsC);
        send(sock, ":", 1, 0);
        int rdwr = 0;
        char gzin[BUFF_SIZE] = {0};
        while(1){
            int status = simpleRead(gfz, gzin, 1024);
            send(sock, gzin, status, 0);
            rdwr += status;
            if(rdwr == gzs)
                break;
        }
        remove(ManPathgz);
        free(sysManPathgz);
        free(ManPathgz);
        close(gfz);
        close(sock);
        pthread_mutex_unlock(proj->lock);
        return NULL;
    }
    send(sock, "16:commandNotFound", 20, 0);
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

int rollbackProject(char* projName, int version){
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
    sprintf(path, "rm -rf %s/commits; mkdir %s/commits", projName, projName);
    system(path);
    sprintf(path, "rm -rf %s/ver%d", projName, currentVer);
    system(path);
    int i;
    for(i = version + 1; i < currentVer; i++){
        sprintf(path, "rm %s/ver%d.tar.gz", projName, i);
        system(path);
    }
    sprintf(path, "%s/ver%d.tar.gz", projName, version);
    unTar(path, projName);
    
    char systemC[BUFF_SIZE];
    sprintf(systemC, "%s/.History", projName);
    int hfd = open(systemC, O_RDWR);
    int hsize = lseek(hfd, 0, SEEK_END);
    lseek(hfd, 0, SEEK_SET);
    char* history = malloc(hsize + 1);
    simpleRead(hfd, history, hsize);
    close(hfd);
    systemC[0] = '\0';
    char ver[BUFF_SIZE];
    sprintf(ver, "Version %d", version + 1);
    for(i = 0; i < hsize; i++){
        if(history[i] == '\n'){
            if(!strcmp(ver, systemC))
                break;
            systemC[0] = '\0';
        }
        else
            strncat(systemC, history + i, 1);
    }
    i = i - strlen(ver);
    sprintf(systemC, "%s/.History", projName);
    hfd = open(systemC, O_RDWR | O_TRUNC);
    simpleWrite(hfd, history, i);
    close(hfd);
    remove(path);
    pthread_mutex_unlock(proj->lock);
    return 1;
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
        pthread_mutex_lock(pLock);
        char commit[BUFF_SIZE] = {0};
        sprintf(commit, "%s/commits", projName);
        mkdir(commit, 0777);
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
        sprintf(path, "%s/.History", projName);
        fd = open(path, O_RDWR | O_CREAT, S_IRWXU);
        close(fd);
        //adding mutex stuff
        PNode * temp = malloc(sizeof(PNode));
        temp->project = malloc(strlen(projName) + 1);
        strcpy(temp->project, projName);
        temp->lock = malloc(sizeof(pthread_mutex_t));
        pthread_mutex_init(temp->lock, NULL);
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
    while(projects != NULL){
        PNode* temp = projects;
        projects = projects->next;
        free(temp->project);
        pthread_mutex_destroy(temp->lock);
        free(temp->lock);
        free(temp);
    }
    exit(0);
}

int main(int argc, char *argv[])
{
	if(argc != 2){
        printf("Fatal error: not enough arguments\n");
        return 0;
    }
    if(!isNumber(argv[1])){
        printf("Fatal error: not a valid port number\n");
        return 0;
    }
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
    int portnum = atoi(argv[1]);
    if(portnum < 0 || portnum > 65535){
        printf("Fatal error: not a valid port number\n");
        return 0;
    }
    acceptClients(portnum);
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
int isNumber(char* in){
    int i;
    for(i = 0; in[i] != '\0'; i++){
    if(!isdigit(in[i]))
        return 0;
    }
    return 1;
}
