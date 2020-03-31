#include <stdlib.h>
#include <stdio.h>
#include "fileCompressor.h"
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

int main(int argc, char** argv){
    printf("%d\n", sizeof(Node));
    int dfd = open("exampleDict.txt", O_RDWR | O_CREAT, S_IRWXU);
    int ofd = open("exampleCompress.txt", O_RDWR | O_CREAT, S_IRWXU);
    int nfd = open("decompress.txt", O_RDWR | O_CREAT, S_IRWXU);
    Node* tree = tokenizeDict(dfd);
    decompressFile(tree, ofd, nfd);
    freeTree(tree);
    return 0;
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
            char* text = temp->value;
            if(!strcmp(text, "\\n"))
                write(nfd, "\n", 1);
            else if(!strcmp(text, "\\t"))
                write(nfd, "\t", 1);
            else if(!strcmp(text, "\\s"))
                write(nfd, " ", 1);
            else
                write(nfd, temp->value, strlen(temp->value));
            temp = tree;
        }
    }

    free(input);
    return;
}

 /*
  * createDictionary
  * Takes 3 arguments, node in the tree, file descriptor for the dictionary,
  * and the local path of the node
  */

void createDictionary(Node* tree, int fd, char* path){
    
}

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
