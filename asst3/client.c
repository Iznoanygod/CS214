#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <signal.h>
#include "client.h"
#include "simpleIO.h"
#define BUFF_SIZE 4096

int main(int argc, char** argv){
    if(argc == 1){
        printf("Fatal error: no argument given\n");
        return 0;
    }
    if(!strcmp(argv[1],"configure")){
        if(argc != 4){
            printf("Fatal error: wrong number of arguments given\n");
            return 0;
        }
        if(!isNumber(argv[3])){
            printf("Fatal error: port number is not valid\n");
            return 0;
        }
        char* host = argv[2];
        char* port = argv[3];
        int fd = open(".configure", O_RDWR | O_CREAT | O_TRUNC, S_IRWXU);
        if(fd == -1){
            printf("Fatal error: error writing to .configure file\n");
            return 0;
        }
        int check = write(fd, host, strlen(host));
        if(check == -1){
            printf("Fatal error: error writing to .configure file\n");
            close(fd);
            return 0;
        }
        check = write(fd, " ", 1);
        if(check == -1){
            printf("Fatal error: error writing to .configure file\n");
            close(fd);
            return 0;
        }
        check = write(fd, port, strlen(port));
        if(check == -1){
            printf("Fatal error: error writing to .configure file\n");
            close(fd);
            return 0;
        }
        return 0;
    }
    if(!strcmp(argv[1], "create")){
        if(argc != 3){
            printf("Fatal error: wrong number of arguments given\n");
            return 0;
        }
        struct sockaddr_in serv_addr;
        int sock = socket(AF_INET, SOCK_STREAM, 0);
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(25585);
        inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);
        connect(sock, (struct sockaddr*) &serv_addr, sizeof(serv_addr));
        
        char message[BUFF_SIZE] = {0};
        int i;
        for(i = 0; ; i++){
            read(sock, message + i, 1);
            if(message[i] == ':')
                break;
        }
        message[i] = '\0';
        int len = atoi(message);
        char* serMes = malloc(len + 1);
        simpleRead(sock, serMes, len);
        printf("%s\n", serMes);
        free(serMes);
        sprintf(message, "create:%d:%s", strlen(argv[2]), argv[2]);
        send(sock, message, strlen(message), 0); 
        
        message[0] = '\0';
        for(i = 0; ; i++){
            read(sock, message + i, 1);
            if(message[i] == ':')
                break;
        }
        message[i] = '\0';
        len = atoi(message);
        serMes = malloc(len + 1);
        simpleRead(sock, serMes, len);
        printf("%s\n", serMes);
        free(serMes);
    }
    else if(!strcmp(argv[1], "destroy")){
        if(argc != 3){
            printf("Fatal error: wrong number of arguments given\n");
            return 0;
        }
        struct sockaddr_in serv_addr;
        int sock = socket(AF_INET, SOCK_STREAM, 0);
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(25585);
        inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);
        connect(sock, (struct sockaddr*) &serv_addr, sizeof(serv_addr));

        char message[BUFF_SIZE] = {0};
        int i;
        for(i = 0; ; i++){
            read(sock, message + i, 1);
            if(message[i] == ':')
                break;
        }
        message[i] = '\0';
        int len = atoi(message);
        char* serMes = malloc(len + 1);
        simpleRead(sock, serMes, len);
        printf("%s\n", serMes);
        free(serMes);
        sprintf(message, "destroy:%d:%s", strlen(argv[2]), argv[2]);
        send(sock, message, strlen(message), 0);
        message[0] = '\0';
        for(i = 0; ; i++){
            read(sock, message + i, 1);
            if(message[i] == ':')
                break;
        }
        message[i] = '\0';
        len = atoi(message);
        serMes = malloc(len + 1);
        simpleRead(sock, serMes, len);
        printf("%s\n", serMes);
        free(serMes);
    }
}
bool isNumber(char* in){
    int i;
    for(i = 0; in[i] != '\0'; i++){
        if(!isdigit(in[i]))
            return false;
    }
    return true;
}
