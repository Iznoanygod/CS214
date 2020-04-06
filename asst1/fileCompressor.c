#include <stdlib.h>
#include <stdio.h>
#include "fileCompressor.h"
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <dirent.h>

int recursive;

/*
 * mode
 * 0 - build codebook
 * 1 - compress
 * 2 - decompress
 */

int mode;

int codebook;
char escapechar;

int main(int argc, char** argv){
    if(argc < 3){
        printf("Fatal Error: Not enough flags\n");
        return 0;
    }
    recursive = !strcmp("-R", argv[1]) ? 1 : 0;
    char* modeFlag = argv[1+recursive];
    if(!strcmp("-R", modeFlag)){
        printf("Fatal Error: Order of flags incorrect. Recursive flag must be first\n");
        return 0;
    }
    if(!strcmp("-b", modeFlag)){
        mode = 0;
    }
    else if(!strcmp("-c", modeFlag)){
        mode = 1;
    }
    else if(!strcmp("-d", modeFlag)){
        mode = 2;
    }
    else{
        printf("Fatal Error: Invalid flag\n");
        return 0;
    }
    if(argc > 4+recursive){
        printf("Fatal Error: Too many flags\n");
        return 0;
    }
    char* desc = argv[2+recursive];
    int codebookFD;
    if(argc == 4+recursive){
        codebook = 1;
        char* cbdesc = argv[3+recursive];
        codebookFD = mode != 0 ? open(cbdesc, O_RDWR) : open(cbdesc, O_RDWR | O_CREAT, S_IRWXU);
        if(codebookFD == -1){
            switch(errno){
                case ENOENT:
                    printf("Fatal Error: Specified codebook does not exist\n");
                    break;
                case EISDIR:
                    printf("Fatal Error: File path given for codebook is a directory\n");
                    break;
                default:
                    printf("Fatal Error: Unspecified error occured while opening codebook\n");
                    break;
            }

            return 0;
        }
        
    }
    else{
        if(mode){
            printf("Fatal Error: No codebook specified\n");
            return 0;
        }
        else{
            printf("Warning: No codebook specified, will use default codebook\n");
            codebookFD = open("HuffmanCodebook", O_RDWR | O_CREAT, S_IRWXU);
        }
    }
    //compress
    if(mode == 1){
        if(recursive){


        }
    }
    //decompress
    else if(mode == 2){
        if(recursive){
            File* files = recurseFiles(desc);
            if(files == NULL){
                printf("Warning: Directory is empty\n");
            }
            File* temp = files;
            Node* tree = tokenizeDict(codebookFD);
            while(temp!= NULL){
                char *last_four = &temp->path[strlen(temp->path)-4];
                if(strcmp(last_four, ".hcz")){
                    printf("Warning: File is not an hcz file, skipping\n");
                    temp = temp->next;
                    continue;
                }
                char* newpath = malloc(strlen(temp->path)-3);
                strncpy(newpath, temp->path, strlen(temp->path)-4);
                newpath[strlen(temp->path) - 4] = '\0';
                int nfd = open(newpath, O_RDWR | O_CREAT, S_IRWXU);
                decompressFile(tree, temp->fd, nfd);
                temp = temp->next;
                free(newpath);
            }
            temp = files;
            while(temp != NULL){
                File *trail = temp;
                temp = temp->next;
                free(trail->path);
                free(trail);
            }
            freeTree(tree);
        }
        else{
            char *last_four = &desc[strlen(desc)-4];
            if(strcmp(last_four, ".hcz")){
                printf("Fatal Error: File is not an hcz file\n");
                exit(0);
            }
            Node* tree = tokenizeDict(codebookFD);
            char* newpath = malloc(strlen(desc) - 3);
            strncpy(newpath, desc, strlen(desc) - 4);
            newpath[strlen(desc) - 4] = '\0';
            int ofd = open(desc, O_RDWR);
            int nfd = open(newpath, O_RDWR | O_CREAT, S_IRWXU);
            decompressFile(tree, ofd, nfd);
            freeTree(tree);
            free(newpath);
        }
    }
    //building codebook
    else{
        Node** arr = NULL;
        int size = 0;
        if(recursive){
            //recursively read files and build codebook
            File* files = recurseFiles(desc);
            File* temp = files;

            while(temp != NULL){
                size = readFile(temp->fd, &arr, size);    
                temp = temp->next;
            }
            temp = files;
            while(temp != NULL){
                File* trail = temp;
                temp = temp->next;
                free(trail->path);
                free(trail);
            }
            free(files);

        }
        else{
            //has 1 file
            int fd = open(desc, O_RDWR);
            if(fd == -1){
                switch(errno){
                    case ENOENT:
                        printf("Fatal Error: Specified file does not exist\n");
                        close(codebookFD);
                        return 0;
                    case EISDIR:
                        printf("Fatal Error: File path given is a directory\n");
                        close(codebookFD);
                        return 0;
                    default:
                        printf("Fatal Error: Unspecified error occured while opening file\n");
                        close(codebookFD);
                        return 0;
                }
            } 
            size = readFile(fd, &arr, 0);
        }
        Node* tree = createTree(arr, size);
        createDictionary(tree, codebookFD);
        free(arr);
        freeTree(tree);
    }
    
    return 0;
}

/*
 * createTree
 * will create full huffmann tree out of array
 */

Node* createTree(Node** arr, int size){
    
}

/*
 * readFile
 * takes 3 arguments, file descriptor, node array, and size of array
 * returns new size of the array
 */

int readFile(int fd, Node*** arr, int size){
    Node** array = *arr;
    int fsize = lseek(fd, 0, SEEK_END);
    lseek(fd, 0, SEEK_SET);
    char* input = malloc(fsize);
    int readin = 0;
    while(1){
        int status = read(fd, input + readin, fsize-readin);
        if(status == -1){
            printf("Fatal Error: Error while reading file\n");
            free(input);
            return -1;
        }
        readin += status;
        if(readin == fsize)
            break;
    }
    close(fd);
    char* string = malloc(fsize);
    string[0] = '\0';
    int i;
    for(i = 0; i < fsize; i++){
        char* token = malloc(fsize);
        token[0] = '\0';
        if(input[i] == '\n' || input[i] == ' ' || input[i] == '\t'){
            sscanf(string, "%s", token);
            int j;
            int check = 0;
            for(j = 0; j < size; j++){
                if(!strcmp(token, array[j]->value)){
                    check = 1;
                    array[j]->frequency++;
                }
            }
            if(!check && strcmp(token, "")){
                Node* temp = malloc(sizeof(Node));
                temp->left = NULL;
                temp->right = NULL;
                temp->frequency = 1;
                temp->value = malloc(strlen(token) + 1);
                strcpy(temp->value, token);
                Node** tarr = malloc(sizeof(Node) * (size+ 1));
                memcpy(tarr, array, sizeof(Node) * size);
                free(array);
                tarr[size] = temp;
                array = tarr;
                size++;
            }
            
            //insert white space
            check = 0;
            for(j = 0; j < size; ++j){
                if(!strncmp(input + i, array[j]->value,1)){
                    check = 1;
                    array[j]->frequency++;
                }
            }
            if(!check){
                Node* temp = malloc(sizeof(Node));
                temp->left = NULL;
                temp->right = NULL;
                temp->frequency = 1;
                temp->value = malloc(2);
                temp->value[0] = input[i];
                temp->value[1] = '\0';
                Node** tarr = malloc(sizeof(Node) * (size+1));
                memcpy(tarr, array, sizeof(Node) * size);
                free(array);
                tarr[size] = temp;
                array = tarr;
                size++;
            }
            string[0] = '\0';
        }
        else{
            strncat(string, input + i, 1);
        }
        free(token);
    }
    *arr = array;
    free(input);
    free(string);
    return size;
}

/*
 * tokenizeDict
 * Returns pointer to huffman tree
 * Non-leaf nodes contain no value
 * All nodes do not have a frequency value
 */

Node* tokenizeDict(int fd){
    Node* tree = malloc(sizeof(Node));
    tree->value = NULL;
    tree->left = NULL;
    tree->right = NULL;
    char* line = malloc(32);
    if(line == NULL){
        printf("Fatal Error: Failed to allocate memory\n");
        close(fd);
        exit(0);
    }
    int buffersize = 32;
    int length = 0;
    line[0] = '\0';

    int size = lseek(fd, 0, SEEK_END);
    lseek(fd, 0, SEEK_SET);
    char* input = malloc(size);
    if(input == NULL){
        printf("Fatal Error: Failed to allocate memory\n");
        free(line);
        close(fd);
        exit(0);
    }
    int readin = 0;
    while(1){
        int status = read(fd, input + readin, size-readin);
        if(status == -1){
            printf("Fatal Error: Error number %d while reading file\n", errno);
            free(input);
            free(line);
            exit(0);
        }
        readin += status;
        if(readin == size)
            break;
    }
    close(fd);
    
    int i;
    for(i = 0; i < size; i++){
        if(input[i] == '\n'){
            if(i == 1){
                escapechar = input[0];
                line[0] = '\0';
                length = 0;
                continue;
            }
            char* binary = malloc(buffersize);
            char* string = malloc(buffersize);
            if(binary == NULL || string == NULL){
                printf("Fatal Error: Failed to allocate memory\n");
                free(input);
                free(line);
                exit(0);
            }
            Node* temp = tree;
            sscanf(line, "%s\t%s", binary, string);
            int j;
            for(j = 0; j < strlen(binary); j++){
                if(binary[j] == '0'){
                    if(temp->left == NULL){
                        temp->left = malloc(sizeof(Node));
                        temp->value = NULL;
                        temp->left->left = NULL;
                        temp->left->right = NULL;
                    }
                    temp = temp->left;
                }
                else{
                    if(temp->right == NULL){
                        temp->right = malloc(sizeof(Node));
                        temp->value = NULL;
                        temp->right->left = NULL;
                        temp->right->right = NULL;
                    }
                    temp = temp->right;
                }
            }
            temp->value = string;
            temp->left = NULL;
            temp->right = NULL;
            
            line[0] = '\0';
            free(binary);
            length = 0;
        }
        else{
            strncat(line, input + i, 1);
            ++length;
            if((length+1) == buffersize){
                char* repl = malloc(buffersize * 2);
                memcpy(repl, line, buffersize);
                buffersize = buffersize * 2;
                free(line);
                line = repl;
            }
        }
    }
    free(line);
    free(input);
    return tree;
}

 /*
  * stringToken
  * Takes a string as the only argument
  * converts it into the proper string representation for
  * codebook
  */

char* stringToken(char* token){
    char* ret = malloc(1+strlen(token));
    int i;
    int j = 0;
    for(i = 0; i < strlen(token); i++){
        if(token[i] == escapechar){
            ++i;
            switch(token[i]){
                case 0x5c:
                    ret[j] = '\\';
                    ++j;
                    break;
                case 0x6e:
                    ret[j] = '\n';
                    ++j;
                    break;
                case 0x73:
                    ret[j] = ' ';
                    ++j;
                    break;
                case 0x74:
                    ret[j] = '\t';
                    ++j;
                    break;
                default:
                    break;
            }
        }
        else{
            ret[j] = token[i];
            ++j;
        }
    }
    ret[j] = '\0';
    return ret;
}

 /*
  * decompressFile
  * Takes 3 arguments, pointer to root of huffman tree, 
  * file descriptor for .hcz file, file descriptor for new file
  */

void decompressFile(Node* tree, int ofd, int nfd){
    int buffersize = 32;
    int length = 0;

    int size = lseek(ofd, 0, SEEK_END);
    lseek(ofd, 0, SEEK_SET);
    char* input = malloc(size);
    if(input == NULL){
        printf("Fatal Error: Failed to allocate memory\n");
        close(ofd);
        close(nfd);
        exit(0);
    }
    int readin = 0;
    while(1){
        int status = read(ofd, input + readin, size - readin);
        if(status == -1){
            printf("Fatal Error: Error number %d while reading file\n", errno);
            free(input);
            exit(0);
        }
        readin += status;
        if(readin == size)
            break;
    }
    close(ofd);

    Node* temp = tree;
    int i;
    for(i = 0; i < size; i++){
        if(input[i] == '0')
            temp = temp->left;
        else
            temp = temp->right;
        if(temp->left == NULL){
            //output value to the file
            char* text = stringToken(temp->value);
            write(nfd, text, strlen(text));
            free(text);
            temp = tree;
        }
    }

    free(input);
    return;
}

/* 
 * compressFile
 * Takes 3 arguments, the dictionary file, the old fild descriptor, and new file descriptor
 */

void compressFile(int cbfd, int ofd, int nfd){
    
}

 /*
  * recurseCreate
  * Used as helper function for createDictionary, takes 3 arguments,
  * node in the tree, file descriptor for the dictionary,
  * and the local path of the node
  */

void recurseCreate(Node* tree, int fd, char* path){
    if(tree->left == NULL){
        write(fd, path, strlen(path));
        write(fd, "\t", 1);
        write(fd, tree->value, strlen(tree->value));
        write(fd, "\n", 1);
        return;
    }
    else{
        char left[256];
        strcpy(left, path);
        strcat(left, "0");
        char right[256];
        strcpy(right, path);
        strcat(right, "1");
        recurseCreate(tree->left, fd, left);
        recurseCreate(tree->right, fd, right);
    }
}

/* 
 * recurseFiles
 * will recurse given directory and returns all files in the form of file struct
 * must remember to free the list of files and path variable
 */

File* recurseFiles(char* path){
    char* currentpath = malloc(512);
    struct dirent* dp;
    DIR* dir = opendir(path);
    //include dir fail check

    File* files = NULL;

    if(!dir){
        free(currentpath);
        return NULL;
    }
    while((dp = readdir(dir)) != NULL){
        if (strcmp(dp->d_name, ".") != 0 && strcmp(dp->d_name, "..") != 0){
            // create file structure
            printf("%s/%s\n", path, dp->d_name);
            char* tpath = malloc(512);
            sprintf(tpath, "%s/%s", path, dp->d_name);
            int fd = open(tpath, O_RDWR);
            if(fd != -1){
                File* temp = malloc(sizeof(File));
                temp->path = tpath;
                temp->fd = fd;
                temp->next = files;
                files = temp;
            }
            strcpy(currentpath, path);
            strcat(currentpath, "/");
            strcat(currentpath, dp->d_name);

            File* rT = recurseFiles(currentpath);
            if(rT != NULL){
                File* ten = rT;
                while(ten->next != NULL)
                    ten = ten->next;
                ten->next = files;
                files = ten;
            }
        }
    }
    free(currentpath);
    closedir(dir);
    return files;
}

 /*
  * createDictionary
  * Takes 3 arguments, node in the tree, file descriptor for the dictionary,
  * and the local path of the node
  */

void createDictionary(Node* tree, int fd){
    write(fd, "\\\n", 2);
    recurseCreate(tree->left, fd, "0");
    recurseCreate(tree->right, fd, "1");
    close(fd);
}

/*
 * freeTree
 * takes the root of the tree as the only paramter
 * will free all memory allocations in the tree
 */

void freeTree(void* root){
    Node* tree = (Node*) root;
    if(tree == NULL)
        return;
    if(tree->value != NULL)
        free(tree->value);
    freeTree(tree->left);
    freeTree(tree->right);
    free(tree);
}
