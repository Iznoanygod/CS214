#ifndef _CLIENT_H
#define _CLIENT_H 1
typedef enum {false,true} bool;
bool isNumber(char* in);
char ** readConfig();
void writeConfig(char *host, char *port);
int getSocket(char *ip, char *port_s);
void sig_handler(int sig);
char * formatMessage(char *delim, int argc, char **args);
int getTotalCharLength(int argc, char **args);
char * itoa(int i);

#endif
