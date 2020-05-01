#ifndef _CLIENT_H
#define _CLIENT_H 1
typedef enum {false,true} bool;
bool isNumber(char* in);
void readConfig();
void writeConfig(char *host, char *port);
int getSocket(char *ip, int port);
void sig_handler(int sig);

#endif
