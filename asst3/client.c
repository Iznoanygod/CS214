#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <ctype.h>
#include "client.h"

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
        int fd = open(".configure", O_RDWR | O_CREAT, S_IRWXU);
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
}
bool isNumber(char* in){
    int i;
    for(i = 0; in[i] != '\0'; i++){
        if(!isdigit(in[i]))
            return false;
    }
    return true;
}
