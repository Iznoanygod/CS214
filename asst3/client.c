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

void readConfig(char ip[], char port[])
{
    int fd = open (".configure", O_RDONLY);
    if(fd == -1)
    {
        printf("Fatal error: error reading .configure file\n");
        exit(EXIT_FAILURE);
    }

    char buf[64];
    int totalBytesRead = 0;
    int bytesRead;
    do
    {
        bytesRead = read(fd, buf+totalBytesRead, 64);
        if (bytesRead == -1)
        {
            printf("Fatal error: error reading .configure file\n");
            close(fd);
            exit(EXIT_FAILURE);
        }
        totalBytesRead+=bytesRead;
    } while(bytesRead != 0);

    buf[totalBytesRead] = '\0';
    char *token = strtok(buf, " ");
    int i;
    for (i = 0; i < 2 && token!=NULL; i++)
    {
        //ret[i] = token;
        if (i==0) strcpy(ip,token);
        if (i==1) strcpy(port,token);
        token = strtok(NULL, " ");
    }
}

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
    else{
        if(access(".configure", F_OK ) == -1) {
            printf("Fatal error: configuration file does not exist\n");
            return 0;
        }
    }
    char ip[BUFF_SIZE];
    char portC[BUFF_SIZE];
    readConfig(ip, portC);
    int port = atoi(portC);
    if(!strcmp(argv[1], "update")){
        if(argc != 3){
            printf("Fatal error: wrong number of arguments given\n");
            return 0;
        }
        char* projName = argv[2];
        
        int sock = conServer(ip, port);
        while(sock == -1){
            sleep(3);
            sock = conServer(ip, port);
        }
        
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
        sprintf(outbound, "update:%d:%s", strlen(argv[2]), argv[2]);
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
        if(localVersion == serverVersion){
            printf("Local project is up to date\n");
            sprintf(systemC, "%s/.Update", argv[2]);
            int mdfd = open(systemC, O_CREAT | O_TRUNC | O_RDWR);
            close(mdfd);
            sprintf(systemC, "%s/.Conflict", argv[2]);
            remove(systemC);
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
        free(line);
        line = malloc(tsize + 1);
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
        free(manifest);
        free(contents);
        free(line);
        int isConflict = 0;
        sprintf(systemC, "%s/.Update", argv[2]);
        int commitFD = open(systemC, O_RDWR | O_CREAT | O_TRUNC, S_IRWXU);
        while(LfileList != NULL || SfileList != NULL){
            if(SfileList == NULL){
                sprintf(systemC, "%s/%s", argv[2], LfileList->path);
                sprintf(outbound, "D %s\n", LfileList->path);
                printf("D %s\n", LfileList->path);
                simpleWrite(commitFD, outbound, strlen(outbound));
                FNode* temp = LfileList;
                LfileList = LfileList->next;
                free(temp->path);
                free(temp->hash);
                free(temp);
                continue;
            }
            else if(LfileList == NULL){
                sprintf(systemC, "%s/%s", argv[2], SfileList->path);
                sprintf(outbound, "A %s\n", SfileList->path);
                printf("A %s\n", SfileList->path);
                simpleWrite(commitFD, outbound, strlen(outbound));
                FNode* temp = SfileList;
                SfileList = SfileList->next;
                free(temp->path);
                free(temp->hash);
                free(temp);
                continue;
            }
            else if(strcmp(LfileList->path, SfileList->path) > 0){
                sprintf(systemC, "%s/%s", argv[2], LfileList->path);
                sprintf(outbound, "D %s\n", LfileList->path);
                printf("D %s\n", LfileList->path);
                simpleWrite(commitFD, outbound, strlen(outbound));
                FNode* temp = LfileList;
                LfileList = LfileList->next;
                free(temp->path);
                free(temp->hash);
                free(temp);
                continue;
            }
            else if(strcmp(LfileList->path, SfileList->path) < 0){
                sprintf(systemC, "%s/%s", argv[2], SfileList->path);
                sprintf(outbound, "A %s\n", SfileList->path);
                printf("A %s\n", SfileList->path);
                simpleWrite(commitFD, outbound, strlen(outbound));
                FNode* temp = SfileList;
                SfileList = SfileList->next;
                free(temp->path);
                free(temp->hash);
                free(temp);
                continue;
            }
            else if(!strcmp(LfileList->path, SfileList->path)){
                if(strcmp(LfileList->hash, SfileList->hash)){
                    sprintf(systemC, "%s/%s", argv[2], LfileList->path);
                    char* md5sum = md5(systemC);
                    if(!strcmp(md5sum, LfileList->hash) && LfileList->version != SfileList->version){
                        sprintf(outbound, "M %s\n", LfileList->path);
                        printf("M %s\n", LfileList->path);
                        simpleWrite(commitFD, outbound, strlen(outbound));
                    }
                    else{
                        isConflict = 1;
                        sprintf(outbound, "%s/.Conflict", projName);
                        int conflictFile = open(outbound, O_CREAT | O_RDWR | O_APPEND, S_IRWXU);
                        sprintf(outbound, "C %s %s\n", LfileList->path, md5sum);
                        write(conflictFile, outbound, strlen(outbound));
                        close(conflictFile);
                        printf("Error: there are conflicts that exist. Please resolve these conflicts before upgrading\n");
                    }
                    free(md5sum);
                }
                
                FNode* temp1 = SfileList;
                FNode* temp2 = LfileList;
                SfileList = SfileList->next;
                LfileList = LfileList->next;
                free(temp1->path);
                free(temp1->hash);
                free(temp2->path);
                free(temp2->hash);
                free(temp1);
                free(temp2);
                continue;
            }
        }
        close(commitFD);
        close(sock);
        if(isConflict){
            sprintf(systemC, "%s/.Update", projName);
            remove(systemC);
        }
        return 0;
    }
    if(!strcmp(argv[1], "upgrade")){
        if(argc != 3){
            printf("Fatal error: wrong number of arguments given\n");
            return 0;
        }
        char outbound[BUFF_SIZE] = {0};
        char inbound[BUFF_SIZE] = {0};
        char systemC[BUFF_SIZE] = {0};

        char* conflict = malloc(512);
        sprintf(conflict, "%s/.Conflict", argv[2]);
        if(access(conflict, F_OK) != -1){
            free(conflict);
            printf("Fatal error: please resolve any conflicts before commiting\n");
            return 0;
        }
        sprintf(conflict, "%s/.Update", argv[2]);
        if(access(conflict, F_OK ) == -1) {
            printf("Fatal error: update file does not exist\n");
            return 0;
        }
        int ufd = open(conflict, O_RDONLY);
        int size = lseek(ufd, 0, SEEK_END);
        lseek(ufd, 0, SEEK_SET);
        close(ufd);
        if(size == 0){
            remove(conflict);
            printf("Project is up to date\n");
            return 0;
        }
        int sock = conServer(ip, port);
        while(sock == -1){
            sleep(3);
            sock = conServer(ip, port);
        }

        sprintf(systemC, "gzip -k %s/.Update", argv[2]);
        system(systemC);
        sprintf(systemC, "%s/.Update.gz", argv[2]);
        ufd = open(systemC, O_RDWR);
        int Usize = lseek(ufd, 0 ,SEEK_END);
        lseek(ufd, 0, SEEK_SET);
        char* updateFile = malloc(Usize + 1);
        simpleRead(ufd, updateFile, Usize);
        remove(systemC);
        free(conflict);
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

        sprintf(outbound, "upgrade:%d:%s", strlen(argv[2]), argv[2]);
        send(sock, outbound, strlen(outbound), 0);
        for(i = 0; ; i++){
            read(sock, inbound + i, 1);
            if(inbound[i] == ':')
                break;
        }
        inbound[i] = '\0';
        size = atoi(inbound);
        simpleRead(sock, inbound, size);
        if(strcmp(inbound, "continue")){
            mesHandle(inbound);
            close(sock);
            close(ufd);
            return 0;
        }
        sprintf(systemC, "%d:", Usize);
        send(sock, systemC, strlen(systemC), 0);
        send(sock, updateFile, Usize, 0);
        
        for(i = 0; ; i++){
            read(sock, inbound + i, 1);
            if(inbound[i] == ':')
                break;
        }
        inbound[i] = '\0';
        len = atoi(inbound);
        char* chgz = malloc(len + 1);
        simpleRead(sock, chgz, len);
        sprintf(systemC, "%s/.Changes.tar.gz", argv[2]);
        int chgzfd = open(systemC, O_CREAT | O_RDWR, S_IRWXU);
        simpleWrite(chgzfd, chgz, len);
        close(chgzfd);
        sprintf(systemC, "cd %s; tar xzf .Changes.tar.gz; rm .Changes.tar.gz", argv[2], argv[2]);
        system(systemC);
        sprintf(systemC, "cp %s/.Changes/.Manifest %s", argv[2], argv[2]); 
        system(systemC);
        sprintf(systemC, "%s/.Update", argv[2]);
        ufd = open(systemC, O_RDWR);
        Usize = lseek(ufd, 0, SEEK_END);
        lseek(ufd, 0, SEEK_SET);
        updateFile = malloc(Usize+1);
        simpleRead(ufd, updateFile, Usize);
        close(ufd);
        remove(systemC);
        inbound[0] = '\0';
        for(i = 0; i < Usize; i++){
            if(updateFile[i] == '\n'){
                char action;
                char* filePath = malloc(Usize);
                sscanf(inbound, "%c %s", &action, filePath);
                if(action == 'D'){
                    sprintf(systemC, "rm %s/%s", argv[2], filePath);
                    system(systemC);
                }
                else{
                    sprintf(systemC, "cp %s/.Changes/%s %s", argv[2], filePath, argv[2]);
                    system(systemC);
                }
                free(filePath);
                inbound[0] = '\0';
            }
            else
                strncat(inbound, updateFile+i, 1);
        }
        close(sock);
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
        int sock = conServer(ip, port);
        while(sock == -1){
            sleep(3);
            sock = conServer(ip, port);
        }
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
        free(line);
        line = malloc(tsize + 1);
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
        free(manifest);
        free(contents);
        free(line);
        int isConflict = 0;
        sprintf(systemC, "%s/.Commit", argv[2]);
        int commitFD = open(systemC, O_RDWR | O_CREAT | O_TRUNC, S_IRWXU);
        while(LfileList != NULL || SfileList != NULL){
            if(SfileList == NULL){
                sprintf(systemC, "%s/%s", argv[2], LfileList->path);
                char* md5sum = md5(systemC);
                sprintf(outbound, "A %s %s\n", LfileList->path, md5sum);
                printf("A %s\n", LfileList->path);
                free(md5sum);
                simpleWrite(commitFD, outbound, strlen(outbound));
                FNode* temp = LfileList;
                LfileList = LfileList->next;
                free(temp->path);
                free(temp->hash);
                free(temp);
                continue;
            }
            else if(LfileList == NULL){
                sprintf(systemC, "%s/%s", argv[2], SfileList->path);
                char* md5sum = md5(systemC);
                sprintf(outbound, "R %s %s\n", SfileList->path, md5sum);
                printf("R %s\n", SfileList->path);
                free(md5sum);
                simpleWrite(commitFD, outbound, strlen(outbound));
                FNode* temp = SfileList;
                SfileList = SfileList->next;
                free(temp->path);
                free(temp->hash);
                free(temp);
                continue;
            }
            else if(strcmp(LfileList->path, SfileList->path) > 0){
                sprintf(systemC, "%s/%s", argv[2], LfileList->path);
                char* md5sum = md5(systemC);
                sprintf(outbound, "A %s %s\n", LfileList->path, md5sum);
                printf("A %s\n", LfileList->path);
                free(md5sum);
                simpleWrite(commitFD, outbound, strlen(outbound));
                FNode* temp = LfileList;
                LfileList = LfileList->next;
                free(temp->path);
                free(temp->hash);
                free(temp);
                continue;
            }
            else if(strcmp(LfileList->path, SfileList->path) < 0){
                sprintf(systemC, "%s/%s", argv[2], SfileList->path);
                char* md5sum = md5(systemC);
                sprintf(outbound, "R %s %s\n", SfileList->path, md5sum);
                printf("R %s\n", SfileList->path);
                free(md5sum);
                simpleWrite(commitFD, outbound, strlen(outbound));
                FNode* temp = SfileList;
                SfileList = SfileList->next;
                free(temp->path);
                free(temp->hash);
                free(temp);
                continue;
            }
            else if(!strcmp(LfileList->path, SfileList->path)){
                if(!strcmp(LfileList->hash, SfileList->hash)){
                    sprintf(systemC, "%s/%s", argv[2], LfileList->path);
                    char* md5sum = md5(systemC);
                    if(strcmp(md5sum, SfileList->hash)){
                        sprintf(outbound, "M %s %s\n", LfileList->path, md5sum);
                        printf("M %s\n", LfileList->path);
                        simpleWrite(commitFD, outbound, strlen(outbound));
                    }
                    free(md5sum);
                }
                else{
                    isConflict = 1;
                    printf("Error: client must synch with the server before committing any changes\n");
                }
                FNode* temp1 = SfileList;
                FNode* temp2 = LfileList;
                SfileList = SfileList->next;
                LfileList = LfileList->next;
                free(temp1->path);
                free(temp1->hash);
                free(temp2->path);
                free(temp2->hash);
                free(temp1);
                free(temp2);
                continue;
            }
        }
        close(commitFD);
        if(isConflict){
            send(sock, "13:commitFailure", 16, 0);
            sprintf(systemC, "%s/.Commit", argv[2]);
            remove(systemC);
            close(sock);
            return 0;
        }
        sprintf(systemC, "gzip -k %s/.Commit", argv[2]);
        system(systemC);
        sprintf(systemC, "%s/.Commit.gz", argv[2]);
        commitFD = open(systemC, O_RDONLY);
        int commitsize = lseek(commitFD, 0, SEEK_END);
        lseek(commitFD, 0, SEEK_SET);
        char* commitOut = malloc(commitsize + 1);
        simpleRead(commitFD, commitOut, commitsize);
        remove(systemC);
        send(sock, "13:commitSuccess", 16, 0);
        sprintf(systemC, "%d:", commitsize);
        send(sock, systemC, strlen(systemC), 0);
        send(sock, commitOut, commitsize, 0);
        
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
       
        close(sock);
        free(commitOut);
        close(commitFD);
    }
    if(!strcmp(argv[1], "push")){
        char outbound[BUFF_SIZE] = {0};
        char inbound[BUFF_SIZE] = {0};
        char systemC[BUFF_SIZE] = {0};
        int responseLength = 0;
        char* projName = argv[2];
        sprintf(systemC, "%s/.Commit", projName);
        //live hash check here
        int liveCheck = open(systemC, O_RDONLY);
        if(liveCheck == -1){
            printf("Fatal error: Commit file does not exist\n");
            return 0;
        }
        int liveSize = lseek(liveCheck, 0, SEEK_END);
        lseek(liveCheck, 0, SEEK_SET);
        char* liveCon = malloc(liveSize+1);
        simpleRead(liveCheck, liveCon, liveSize);
        close(liveCheck);
        char* parser = malloc(liveSize+1);
        int p;
        parser[0] = '\0';
        for(p = 0;p < liveSize; p++){
            if(liveCon[p] == '\n'){
                char op;
                char* path = malloc(liveSize);
                char hash[33] = {0};
                sscanf(parser, "%c %s %s", &op, path, hash);
                sprintf(systemC, "%s/%s", projName, path);
                char* md5sum = md5(systemC);
                if(strcmp(md5sum, hash)){
                    printf("Fatal error: file was modified. Please recommit and push again\n");
                    free(md5sum);
                    free(path);
                    free(parser);
                    return 0;
                }
                free(md5sum);
                free(path);

                parser[0] = '\0';
            }
            else{
                strncat(parser, liveCon+p,1);
            }

        }

        int sock = conServer(ip, port);
        while(sock == -1){
            sleep(3);
            sock = conServer(ip, port);
        }
        
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

        send(sock, "push:", 5, 0);
        sprintf(systemC, "%d:%s", strlen(projName), projName);
        send(sock, systemC, strlen(systemC), 0);

        for(i = 0; ;i++){
            int in = read(sock, inbound+i, 1);
            if(inbound[i] == ':')
                break;
        }
        inbound[i] = '\0';
        responseLength = atoi(inbound);
        simpleRead(sock, inbound, responseLength);
        if(strcmp(inbound, "continue")){
            mesHandle(inbound);
            close(sock);
            return 0;
        }
        sprintf(systemC, "gzip -k %s/.Commit", projName);
        system(systemC);
        sprintf(systemC, "%s/.Commit.gz", projName);
        int cgz = open(systemC, O_RDONLY);
        int gzsize = lseek(cgz, 0, SEEK_END);
        lseek(cgz, 0, SEEK_SET);
        char* gzbuffer = malloc(gzsize + 1);
        simpleRead(cgz, gzbuffer, gzsize);
        close(cgz);
        remove(systemC);
        sprintf(outbound, "%d:", gzsize);
        send(sock, outbound, strlen(outbound), 0);
        send(sock, gzbuffer, gzsize, 0);
        for(i = 0; ; i++){
            int in = read(sock, inbound+i, 1);
            if(inbound[i] == ':')
                break;
        }
        inbound[i] = '\0';
        responseLength = atoi(inbound);
        simpleRead(sock, inbound, responseLength);
        if(strcmp(inbound, "continue")){
            mesHandle(inbound);
            close(sock);
            return 0;
        }
        
        sprintf(systemC, "%s/.Manifest", projName);
        int manifestFD = open(systemC, O_RDWR);
        int manifestSize = lseek(manifestFD, 0, SEEK_END);
        lseek(manifestFD, 0, SEEK_SET);
        char* manifest = malloc(manifestSize+1);
        simpleRead(manifestFD, manifest, manifestSize);
        close(manifestFD);
        systemC[0] = '\0';
        for(i = 0; ; i++){
            if(manifest[i] == '\n')
                break;
            strncat(systemC, manifest+i, 1);
        }
        manifest[i] = '\0';
        int manifestVersion = atoi(systemC);
        inbound[0] = '\0';
        FNode* manifestFiles = NULL;
        for(++i; i < manifestSize; i++){
            if(manifest[i] == '\n'){
                int linesize = strlen(inbound);
                FNode* new = malloc(sizeof(FNode));
                new->path = malloc(linesize);
                new->hash = malloc(linesize);
                sscanf(inbound, "%s %d %s", new->path, &(new->version), new->hash);
                new->next = manifestFiles;
                manifestFiles = new;
                inbound[0] = '\0';
            }
            else{
                strncat(inbound, manifest+i,1);
            }
        }
        
        sprintf(systemC, "%s/.Changes", projName);
        mkdir(systemC, 0777);
        char line[BUFF_SIZE] = {0};
        sprintf(systemC, "%s/.Commit", projName);
        int commitFD = open(systemC, O_RDONLY);
        int commitSize = lseek(commitFD, 0, SEEK_END);
        lseek(commitFD, 0, SEEK_SET);
        char* commitFile = malloc(commitSize + 1);
        simpleRead(commitFD, commitFile, commitSize);
        close(commitFD);
        for(i = 0; i < commitSize; i++){
            if(commitFile[i] == '\n'){
                char action;
                char* path = malloc(strlen(line));
                char* hash = malloc(33);
                sscanf(line, "%c %s %s", &action, path, hash);
                if(action == 'A' || action == 'M'){
                    sprintf(systemC, "cd %s; cp --parents %s .Changes", projName, path);
                    system(systemC);
                    if(action == 'M'){
                        FNode* find = manifestFiles;
                        while(strcmp(find->path, path))
                            find = find->next;
                        find->version++;
                        free(find->hash);
                        find->hash = hash;
                    }
                }
            }
            else{
                strncat(line, commitFile+i, 1);
            }
        }
        insertionSort(&manifestFiles);
        sprintf(systemC, "%s/.Manifest", projName);
        manifestFD = open(systemC, O_RDWR | O_TRUNC);
        sprintf(outbound, "%d\n", manifestVersion+1);
        simpleWrite(manifestFD, outbound, strlen(outbound));
        while(manifestFiles != NULL){
            sprintf(outbound, "%s %d %s\n", manifestFiles->path, manifestFiles->version, manifestFiles->hash);
            simpleWrite(manifestFD, outbound, strlen(outbound));
            FNode* temp = manifestFiles;
            manifestFiles = manifestFiles->next;
            free(temp->path);
            free(temp->hash);
            free(temp);
        }
        close(manifestFD);
        sprintf(systemC, "tar czf %s/.Changes.tar.gz %s/.Changes", projName, projName);
        system(systemC);
        sprintf(systemC, "%s/.Changes.tar.gz", projName);
        cgz = open(systemC, O_RDWR);
        int clength = lseek(cgz, 0, SEEK_END);
        lseek(cgz, 0, SEEK_SET);
        char* cfile = malloc(clength+1);
        simpleRead(cgz, cfile, clength);
        sprintf(outbound, "%d:", clength);
        send(sock, outbound, strlen(outbound), 0);
        send(sock, cfile, clength, 0);
        sprintf(systemC, "cd %s; rm -rf .Changes; rm .Changes.tar.gz; rm .Commit", projName);
        system(systemC);
        close(sock);
        printf("Push Successful\n");
    }
    if(!strcmp(argv[1], "checkout")){
        if(argc != 3){
            printf("Fatal error: wrong number of arguments given\n");
            return 0;
        }
        
        DIR* dir = opendir(argv[2]);
        if (dir) {
    /* Directory exists. */
            printf("Fatal error: project exists locally\n");
            closedir(dir);
            return 0;
        }

        int sock = conServer(ip, port);
        while(sock == -1){
            sleep(3);
            sock = conServer(ip, port);
        }


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
        int sock = conServer(ip, port);
        while(sock == -1){
            sleep(3);
            sock = conServer(ip, port);
        }

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
        int sock = conServer(ip, port);
        while(sock == -1){
            sleep(3);
            sock = conServer(ip, port);
        }

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
        int sock = conServer(ip, port);
        while(sock == -1){
            sleep(3);
            int sock = conServer(ip, port);
        }
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
            else{
                strncat(line, buffer+i, 1);
            }
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
            else{
                strncat(line, buffer+i, 1);
            }
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
        int sock = conServer(ip, port);
        while(sock == -1){
            sleep(3);
            sock = conServer(ip, port);
        }

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
    if(!strcmp(argv[1], "history")){
        if(argc != 3){
            printf("Fatal error: wrong number of arguments given\n");
            return 0;
        }
        int sock = conServer(ip, port);
        while(sock == -1){
            sock = conServer(ip, port);
        }
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
        sprintf(message, "history:%d:%s", strlen(argv[2]), argv[2]);
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
        printf("%s", tempC);
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
    if(!strcmp(serMes, "pushFail")){
        printf("Error: push rejected\n");      
    }
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
    if(!strcmp(serMes, "success")){
        printf("Success\n");
    }
}
