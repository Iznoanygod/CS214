#include <stdlib.h>
#include <stdio.h>
#include "fileCompressor.h"
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

int main(int argc, char** argv){
    printf("%d\n", sizeof(Node));
    int fd = open("exampleDict.txt", O_RDWR | O_CREAT, S_IRWXU);
    Node* tree = tokenizeDict(fd);
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
