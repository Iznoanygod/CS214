#ifndef _SERVER_H
#define _SERVER_H 1
#include <pthread.h>

typedef struct TNode
{
	pthread_t tid;
	struct TNode* next;
}TNode;

typedef struct PNode
{
    char* project;
    pthread_mutex_t* lock;
    struct PNode* next;
}PNode;

void acceptClients(int port);
void * handleClient(void * socket);
void collectThreads(TNode *head);
char * itoa(int i);
void createProjectFile();
int createProject(char* projName);
int destroyProject(char* projName);
int rollbackProject(char* projName, int version);
int isNumber(char* in);
#endif
