#ifndef _SERVER_H
#define _SERVER_H 1
#include <pthread.h>

typedef enum {false,true} bool;

typedef struct TNode
{
	pthread_t tid;
	struct TNode* next;
}TNode;

void acceptClients(int port);
void * handleClient(void * socket);
void collectThreads(TNode *head);
char * itoa(int i);

#endif
