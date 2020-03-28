#include <stdlib.h>
#include <stdio.h>
#include "fileCompressor.h"
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

int main(int argc, char** argv){

    return 0;
}

void tokenizeDict(int fd){
    char* word = malloc(32);
    if(word == NULL){
        printf("Fatal Error: Failed to allocate memory\n");
        close(fd);
        exit(0);
    }
    int buffersize = 32;
    int length = 0;
    word[0] = '\0';

    int size = lseek(fd, 0, SEEK_END);
    lseek(fd, 0, SEEK_SET);
    char* input = malloc(size);
    if(input == NULL){
        printf("Fatal Error: Failed to allocate memory\n");
        free(word);
        close(fd);
        exit(0);
    }
    int readin = 0;
    while(1){
        int status = read(fd, input + readin, size-readin);
        if(status == -1){
            printf("Fatal Error: Error number %d while reading file\n", errno);
            free(input);
            free(word);
            exit(0);
        }
        readin += status;
        if(readin == size)
            break;
    }
    close(fd);
    int i;
    for(i = 0; i < size; i++){
        //finish tokenizing here, parsing the file
    
    }
}
