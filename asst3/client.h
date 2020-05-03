#ifndef _CLIENT_H
#define _CLIENT_H 1
typedef enum {false,true} bool;
bool isNumber(char* in);
void readConfig(char ip[], char port[]);
void writeConfig(char *host, char *port);
int getSocket(char *ip, char *port_s);
void sig_handler(int sig);
char * itoa(int i);

#endif
