#include <stdlib.h>
#include <stdio.h>
#include "fileCompressor.h"
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

int recursive;

/*
 * mode
 * 0 - build codebook
 * 1 - compress
 * 2 - decompress
 */

int mode;


int main(int argc, char** argv){
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
    
    printf("%d\n", sizeof(Node));
    int dfd = open("exampleDict.txt", O_RDWR | O_CREAT, S_IRWXU);
    int ofd = open("exampleCompress.txt", O_RDWR | O_CREAT, S_IRWXU);
    int nfd = open("decompress.txt", O_RDWR | O_CREAT, S_IRWXU);
    int ndfd = open("newDict.txt", O_RDWR | O_CREAT, S_IRWXU);
    Node* tree = tokenizeDict(dfd);
    decompressFile(tree, ofd, nfd);
    createDictionary(tree, ndfd);
    freeTree(tree);
    return 0;
}

/*
 * readFile
 * takes 3 arguments, file descriptor, node array, and size of array
 * returns new size of the array
 */

int readFile(int fd, Node*** arr, int size){
    Node** array = *arr;
    
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
        if(token[i] == '\\'){
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
            printf("Fatal ErrorL Error number %d while reading file\n", errno);
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
  * createDictionary
  * Takes 3 arguments, node in the tree, file descriptor for the dictionary,
  * and the local path of the node
  */

void createDictionary(Node* tree, int fd){
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
