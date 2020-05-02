#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
//#include <zlib.h>
//#include <libtar.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "simpleIO.h"

int simpleRead(int fd, char* buffer, int maxRead){
    int size;
    if(maxRead == -1)
        size = lseek(fd, 0, SEEK_END);
    else
        size = maxRead;
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

int simpleWrite(int fd, char* buffer, int maxWrite){
    int size;
    if(maxWrite == -1)
        size = strlen(buffer);
    else
        size = maxWrite;
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

int unTar(char* path, char* loc){
    /*gzFile fi = gzopen(path, "rb");
    gzrewind(fi);
    char* tarPath = malloc(strlen(path));
    strcpy(tarPath, path);
    tarPath[strlen(path) - 3] = '\0';
    int tfd = open(tarPath, O_RDWR | O_CREAT | O_APPEND, S_IRWXU);
    while(!gzeof(fi)){
        char buffer[1024] = {0};
        int zlen = gzread(fi, buffer, 1024);
        if(!simpleWrite(tfd, buffer, 1024)){
            return 0;
        }
    }
    gzclose(fi);
    close(tfd);
    TAR *pTar = NULL;
    tar_open(&pTar, tarPath, NULL, O_RDONLY, 0777, TAR_GNU);
    tar_extract_all(pTar, loc);
    tar_close(pTar);
    remove(tarPath);*/
    char asd[1024] = {0};
    sprintf(asd, "tar xzvf %s %s", path, loc);
    system(asd);
    return 1;

}

int Tar(char* path, char* loc){
    /*TAR *pTar = NULL;
    char* tarPath = malloc(strlen(path) + 5);
    strcpy(tarPath, path);
    strcat(tarPath, ".tar");
    tar_open(&pTar, tarPath, NULL, O_WRONLY | O_CREAT, 0777, TAR_GNU);
    tar_append_tree(pTar, path, loc);
    tar_append_eof(pTar);
    tar_close(pTar);*/
    return 1;
}
