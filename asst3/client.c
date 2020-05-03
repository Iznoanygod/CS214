#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <signal.h>
#include "client.h"
#include <dirent.h>
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
    if(!strcmp(argv[1], "push")){
        struct sockaddr_in serv_addr;
        int sock = socket(AF_INET, SOCK_STREAM, 0);
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(25585);
        inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);
        connect(sock, (struct sockaddr*) &serv_addr, sizeof(serv_addr));
        send(sock, "push:4:proj:10:aaaaaaaaaa", 26, 0); 
    }
    if(!strcmp(argv[1], "checkout")){
        struct sockaddr_in serv_addr;
        int sock = socket(AF_INET, SOCK_STREAM, 0);
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(25585);
        inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);
        connect(sock, (struct sockaddr*) &serv_addr, sizeof(serv_addr));
        
        char init[BUFF_SIZE] = {0};
        int i;
        for(i = 0; ; i++){
            read(sock, init + i, 1);
            if(init[i] == ':')
                break;
        }
        init[i] = '\0';
        int len = atoi(init);
        char* serMes = malloc(len + 1);
        simpleRead(sock, serMes, len);
        printf("%s\n", serMes);
        free(serMes);
        char command[BUFF_SIZE] = {0};
        sprintf(command, "checkout:%d:%s", strlen(argv[2]), argv[2]);
        send(sock, command, strlen(command), 0);
        char message[BUFF_SIZE] = {0};
        for(i = 0; ; i++){
            read(sock, message + i, 1);
            if(message[i] == ':')
                break;
        }
        message[i] = '\0';
        int size = atoi(message);
        char tarPath[BUFF_SIZE] = {0};
        sprintf(tarPath, "%s.tar.gz", argv[2]);
        int tarFD = open(tarPath, O_RDWR | O_CREAT, S_IRWXU);
        char* tarcontents = malloc(size + 1);
        read(sock, tarcontents, size);
        write(tarFD, tarcontents, size);
        close(tarFD);
        sprintf(command, "tar xzf %s.tar.gz; rm %s.tar.gz", argv[2], argv[2]);
        system(command);
        close(sock);
        return 0;
    }
    if(!strcmp(argv[1], "rollback")){
        if(argc != 4){
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
        sprintf(message, "rollback:%d:%s:%d:%s", strlen(argv[2]), argv[2], strlen(argv[3]), argv[3]);
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
        if(!strcmp(serMes, "projectSuccess")){
            mkdir(argv[2], 0777);
            read(sock, serMes, 1);
            char filesize[512] = {0};
            for(i = 0; ; i++){
                read(sock, filesize + i, 1);
                if(filesize[i] == ':')
                    break;
            }
            filesize[i] = '\0';
            int fs = atoi(filesize);
            char* Manifest = malloc(fs+1);
            simpleRead(sock, Manifest, fs);
            char manpath[512];
            sprintf(manpath, "%s/.Manifest.gz", argv[2]);
            int mfd = open(manpath, O_RDWR | O_CREAT, S_IRWXU);
            simpleWrite(mfd, Manifest, fs);
            close(mfd);
            sprintf(manpath, "gunzip %s/.Manifest.gz", argv[2]);
            system(manpath);
            free(Manifest);
        }
        else{
            printf("Failed to create project, ");
        }
        free(serMes);
        return 0;
    }
    if(!strcmp(argv[1], "destroy")){
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
    if(!strcmp(argv[1], "add")){
        if(argc != 4){
            printf("Fatal error: wrong number of arguments given\n");
            return 0;
        }
        DIR* dir = opendir(argv[2]);
        if(!dir){
            printf("Fatal error: project does not exist\n");
            return 0;
        }
        closedir(dir);
        char manifestPath[BUFF_SIZE] = {0};
        sprintf(manifestPath, "%s/.Manifest", argv[2]);
        int mfd = open(manifestPath, O_RDWR);
        if(mfd == -1){
            printf("Fatal error: path is not a valid project\n");
            return 0;
        }
        int msize = lseek(mfd, 0, SEEK_END);
        lseek(mfd, 0, SEEK_SET);
        int version;
        int i;
        char* buffer = malloc(msize+1);
        simpleRead(mfd, buffer, -1);
        char* line = malloc(msize + 1);
        line[0] = '\0';
        for(i = 0; ; i++){
            if(buffer[i] == '\n')
                break;
            strncat(line, buffer+i, 1);
        }
        FNode* fileList = NULL;
        line[i] = '\0';
        version = atoi(line);
        line[0] = '\0';
        for(++i;i < msize; i++){
            if(buffer[i] == '\n'){
                line[i] = '\0';
                int linesize = strlen(line);
                char* FName = malloc(linesize);
                char* FHash = malloc(linesize);
                int FVersion;
                sscanf(line, "%s %d %s", FName, &FVersion, FHash);
                FNode* temp = malloc(sizeof(FNode));
                temp->path = FName;
                temp->hash = FHash;
                temp->version = FVersion;
                temp->next = fileList;
                fileList = temp;
            }
            else
                strncat(line, buffer+i, 1);
        }
        FNode* track = fileList;
        while(track != NULL){
            if(!strcmp(track->path, argv[3]))
                break;
            track = track->next;
        }
        if(track == NULL){
            FNode* temp = malloc(sizeof(FNode));
            temp->path = malloc(strlen(argv[3])+1);
            strcpy(temp->path, argv[3]);
            temp->version = 0;
            temp->hash = md5(argv[3]);
            temp->next = fileList;
            fileList = temp;
        }
        else{
            char* newHash = md5(argv[3]);
            if(strcmp(newHash, track->hash)){
                track->version++;
                free(track->hash);
                track->hash = newHash;
            }
            else{
                free(newHash);
            }
        }
        insertionSort(&fileList);
        close(mfd);
        mfd = open(manifestPath, O_RDWR | O_TRUNC);
        char versionC[512] = {0};
        sprintf(versionC, "%d\n", version);
        simpleWrite(mfd, versionC, -1);
        while(fileList != NULL){
            char output[BUFF_SIZE] = {0};
            sprintf(output, "%s %d %s\n", fileList->path, fileList->version, fileList->hash);
            simpleWrite(mfd, output, -1);
            FNode* temp = fileList;
            fileList = fileList->next;
            free(temp->path);
            free(temp->hash);
            free(temp);
        }
        free(buffer);
        free(line);
        close(mfd);
        return 0;
    }
    if(!strcmp(argv[1], "currentversion")){
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
        sprintf(message, "currentVersion:%d:%s", strlen(argv[2]), argv[2]);
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
        int gfd = open(".temp.gz", O_RDWR | O_CREAT, S_IRWXU);
        simpleWrite(gfd, serMes, len);
        close(gfd);
        system("gunzip .temp.gz");
        int tempfd = open(".temp", O_RDONLY);
        int tempsize = lseek(tempfd, 0, SEEK_END);
        lseek(tempfd, 0, SEEK_SET);
        i = 0;
        char* tempC = malloc(tempsize);
        simpleRead(tempfd, tempC, tempsize);
        close(tempfd);
        remove(".temp");
        for(i = 0; ; i++){
            if(tempC[i] == '\n')
                break;
        }
        char* line = malloc(tempsize);
        line[0] = '\0';
        for(++i; i < tempsize; i++){
            if(tempC[i] == '\n'){
                char* file = malloc(tempsize);
                int ver;
                sscanf(line, "%s %d %*s", file, &ver);
                printf("%s : version %d\n", file, ver);
                line[0] = '\0';
                free(file);
            }
            else{
                strncat(line, tempC+i, 1);
            }
        }
        free(line);
        free(tempC);
        free(serMes);
    }
}
int isNumber(char* in){
    int i;
    for(i = 0; in[i] != '\0'; i++){
        if(!isdigit(in[i]))
            return 0;
    }
    return 1;
}
