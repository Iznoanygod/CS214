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
#include <openssl/md5.h>
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
        if(readin == size || status == 0)
            break;
    }
    buffer[readin] = '\0';
    return readin;
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
        if(writeout == size || status == 0)
            break;
    }
    return writeout;
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
    sprintf(asd, "tar xzf %s %s", path, loc);
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
    char asd[1024] = {0};
    sprintf(asd, "tar czf %s %s", path, loc);
    system(asd);
    return 1;
}

int insertionSort(FNode** toSort){
    FNode* head = *toSort;
    if(head == NULL)
        return 0;
    FNode* list = head->next;
    head->next = NULL;
    while(list != NULL){
        if(strcmp(list->path, head->path) < 0){
            FNode* temp = list;
            list = list->next;
            temp->next = head;
            head = temp;
        }
        else{
            FNode* leading = head->next;
            FNode* trailing = head;
            while(leading != NULL && strcmp(list->path, leading->path) > 1){
                leading = leading->next;
                trailing = trailing->next;
            }
            FNode* temp = list;
            list = list->next;
            trailing->next = temp;
            temp->next = leading;
        }
    }
    FNode** root = toSort;
    *root = head;
    return 0;
}
char* md5(char* path){
    unsigned char c[MD5_DIGEST_LENGTH];
    int fd = open(path, O_RDONLY);
    MD5_CTX mdContext;
    int bytes;
    unsigned char buffer[256];
    if(fd < 0)
        return NULL;
    MD5_Init(&mdContext);
    do{
        bzero(buffer, 256);
        bytes = read(fd, buffer, 256);
        MD5_Update (&mdContext, buffer, bytes);
    }while(bytes > 0);
    MD5_Final(c, &mdContext);
    close(fd);
    int i;
    char* hash = malloc(33);
    bzero(hash, 32);
    char buf[3];
    for(i = 0; i < MD5_DIGEST_LENGTH; i++){
        sprintf(buf, "%02x", c[i]);
        strcat(hash, buf);
    }
    return hash;
}
