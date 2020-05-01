#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include "simpleIO.h"

int simpleRead(int fd, char* buffer){
    int size = lseek(fd, 0, SEEK_END);
    lseek(fd, 0, SEEK_SET);
    int readin = 0;
    while(1){
        int status = read(fd, buffer+readin, size-readin);
        if(status == -1){
            return 0;
        }
        readin += status;
        if(readin == size)
            break;
    }
    buffer[readin] = '\0';
    return 1;
}

int simpleWrite(int fd, char* buffer){
    int size = strlen(buffer);
    int writeout = 0;
    while(1){
        int status = write(fd, buffer+writeout, size-writeout);
        if(status == -1){
            return 0;
        }
        writeout += status;
        if(writeout == size)
            break;
    }
    return 1;
}
