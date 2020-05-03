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

int conServer(char* ip, int port){
    struct sockaddr_in serv_addr;
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    inet_pton(AF_INET, ip, &serv_addr.sin_addr);
    connect(sock, (struct sockaddr*) &serv_addr, sizeof(serv_addr));
    return sock;
}
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
    if(!strcmp(argv[1], "commit")){
        if(argc != 3){
            printf("Fatal error: wrong number of arguments given\n");
            return 0;
        }
        char* conflict = malloc(512);
        sprintf(conflict, "%s/.Conflict", argv[2]);
        if(access(conflict, F_OK ) != -1 ) {
            free(conflict);
            printf("Fatal error: please resolve any conflicts before commiting\n");
            return 0;
        }
        sprintf(conflict, "%s/.Update", argv[2]);
        if(access(conflict, F_OK ) != -1) {
            int ufd = open(conflict, O_RDONLY);
            int size = lseek(ufd, 0, SEEK_END);
            if(size){
                printf("Fatal error: please upgrade before commiting\n");
                return 0;
            }
        }
        free(conflict);
        int sock = conServer("127.0.0.1", 25585);
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
        mesHandle(serMes);
        free(serMes);
        
        char outbound[BUFF_SIZE] = {0};
        char inbound[BUFF_SIZE] = {0};
        char systemC[BUFF_SIZE] = {0};
        sprintf(outbound, "commit:%d:%s", strlen(argv[2]), argv[2]);
        send(sock, outbound, strlen(outbound), 0);
        for(i = 0; ; i++){
            read(sock, inbound + i, 1);
            if(inbound[i] == ':')
                break;
        }
        inbound[i] = '\0';
        int size = atoi(inbound);
        if(!size){
            for(i = 0; ; i++){
                read(sock, inbound + i, 1);
                if(inbound[i] == ':')
                    break;
            }
            inbound[i] = '\0';
            int flen = atoi(inbound);
            char fmes[BUFF_SIZE] = {0};
            simpleRead(sock, fmes, flen);
            mesHandle(fmes);
            close(sock);
            return 0;
        }
        char* contents = malloc(size + 1);
        simpleRead(sock, contents, size);
        int tempFD = open(".temp.gz", O_CREAT | O_RDWR, S_IRWXU);
        simpleWrite(tempFD, contents, size);
        close(tempFD);
        free(contents);
        system("gunzip .temp.gz");
        tempFD = open(".temp", O_RDWR);
        int tsize = lseek(tempFD, 0, SEEK_END);
        lseek(tempFD, 0, SEEK_SET);
        contents = malloc(tsize + 1);
        simpleRead(tempFD, contents, tsize);
        close(tempFD);
        remove(".temp");
        for(i = 0; ; i++){
            inbound[i] = contents[i];
            if(inbound[i] == '\n')
                break;
        }
        inbound[i] = '\0';
        int serverVersion = atoi(inbound);
        int serverLock = i;
        sprintf(systemC, "%s/.Manifest", argv[2]);
        int mfd = open(systemC, O_RDWR);
        int msize = lseek(mfd, 0, SEEK_END);
        lseek(mfd, 0, SEEK_SET);
        char* manifest = malloc(msize + 1);
        simpleRead(mfd, manifest, msize);
        close(mfd);
        for(i = 0; ; i++){
            inbound[i] = manifest[i];
            if(inbound[i] == '\n')
                    break;
        }
        inbound[i] = '\0';
        int localVersion = atoi(inbound);
        if(localVersion != serverVersion){
            printf("Fatal error: please update your local project\n");
            free(manifest);
            free(contents);
            close(sock);
            return 0;
        }
        FNode* LfileList = NULL;
        FNode* SfileList = NULL;
        char* line = malloc(msize + 1);
        line[0] = '\0';
        for(++i;i < msize; i++){
            if(manifest[i] == '\n'){
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
                temp->next = LfileList;
                LfileList = temp;
                line[0] = '\0';
            }
            else
                strncat(line, manifest+i, 1);
        }
        line[0] = '\0';
        for(i = serverLock+1;i < tsize; i++){
            if(contents[i] == '\n'){
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
                temp->next = SfileList;
                SfileList = temp;
                line[0] = '\0';
            }
            else
                strncat(line, contents+i, 1);
        }
        free(line);
        int isConflict = 0;
        sprintf(systemC, "%s/.Commit", argv[2]);
        int commitFD = open(systemC, O_RDWR | O_CREAT | O_TRUNC, S_IRWXU);
        while(LfileList != NULL || SfileList != NULL){
            if(SfileList == NULL || strcmp(LfileList->path, SfileList->path) > 0){
                sprintf(outbound, "A %s\n", LfileList->path);
                printf("A %s\n", LfileList->path);
                simpleWrite(commitFD, outbound, strlen(outbound));
                LfileList = LfileList->next;
                continue;
            }
            if(LfileList == NULL || strcmp(LfileList->path, SfileList->path) < 0){
                sprintf(outbound, "R %s\n", SfileList->path);
                simpleWrite(commitFD, outbound, strlen(outbound));
                SfileList = SfileList->next;
                continue;
            }
            if(!strcmp(LfileList->path, SfileList->path)){
                
            }
        }    
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
        mesHandle(serMes);
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
        if(!size){
            for(i = 0; ; i++){
                read(sock, message + i, 1);
                if(message[i] == ':')
                    break;
            }
            message[i] = '\0';
            int flen = atoi(message);
            char fmes[BUFF_SIZE] = {0};
            simpleRead(sock, fmes, flen);
            mesHandle(fmes);
            close(sock);
            return 0;
        }
        message[i] = '\0';
        
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
        mesHandle(serMes);
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
        
        mesHandle(serMes);

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
        mesHandle(serMes);
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
        mesHandle(serMes);
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
        mesHandle(serMes);
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
        mesHandle(serMes);
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
        char* conflict = malloc(512);
        sprintf(conflict, "%s/%s", argv[2], argv[3]);
        if(access(conflict, F_OK ) == -1 ) {
            free(conflict);
            printf("Fatal error: file does not exist\n");
            return 0;
        }
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
                line[0] = '\0';
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
            char hashF[BUFF_SIZE] = {0};
            sprintf(hashF, "%s/%s", argv[2], argv[3]);
            temp->hash = md5(hashF);
            temp->next = fileList;
            fileList = temp;
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
    if(!strcmp(argv[1], "remove")){
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
        char* conflict = malloc(512);
        sprintf(conflict, "%s/%s", argv[2], argv[3]);
        if(access(conflict, F_OK ) == -1 ) {
            free(conflict);
            printf("Fatal error: file does not exist\n");
            return 0;
        }
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
                line[0] = '\0';
            }
            else
                strncat(line, buffer+i, 1);
        }
        FNode* trail = NULL;
        FNode* track = fileList;
        while(track != NULL){
            if(!strcmp(track->path, argv[3]))
                break;
            trail = track;
            track = track->next;
        }
        if(track == NULL);
        else if(trail == NULL){
            FNode* temp = fileList;
            fileList = fileList->next;
            free(temp->path);
            free(temp->hash);
            free(temp);
        }
        else{
            trail->next = track->next;
            free(track->path);
            free(track->hash);
            free(track);
        }
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
        mesHandle(serMes);
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
        if(!len){
            for(i = 0; ; i++){
                read(sock, message + i, 1);
                if(message[i] == ':')
                    break;
            }
            message[i] = '\0';
            int flen = atoi(message);
            char fmes[BUFF_SIZE] = {0};
            simpleRead(sock, fmes, flen);
            mesHandle(fmes);
            close(sock);
            return 0;
        }
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
void mesHandle(char* serMes){
    if(!strcmp(serMes, "messageFail")){
        printf("Error: message to server malformed\n");
    }
    if(!strcmp(serMes, "projectNotExist")){
        printf("Error: project does not exist\n");
    }
    if(!strcmp(serMes, "invalidVersion")){
        printf("Error: invalid project version\n");
    }
    if(!strcmp(serMes, "projectAlreadyExist")){
        printf("Error: project already exists\n");
    }
    if(!strcmp(serMes, "projectError")){
        printf("Error: failed to create project\n");
    }
    if(!strcmp(serMes, "projectSuccess")){
        printf("Command sent successfully\n");
    }
    if(!strcmp(serMes, "clientConnectSuccess")){
        printf("Client connected successfully\n");
    }
}
