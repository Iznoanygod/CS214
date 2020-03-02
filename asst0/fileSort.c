#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "fileSort.h"
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

int main(int argc, char** argv){
    if(argc != 3){
        printf("Fatal Error: Expected two arguments, had %d\n", (argc-1));
        return 0;
    }
    int sortType;
    if(!stringCompare(argv[1], "-q")){
        sortType = 0;
    }
    else if(!stringCompare(argv[1], "-i")){
        sortType = 1;
    }
    else{
        printf("Fatal Error: \"%s\" is not a valid sort flag\n", argv[1]);
        return 0;
    }
    int fd = open(argv[2], O_RDONLY);
    if(fd == -1){
        printf("Fatal Error: file \"%s\" does not exist\n", argv[2]);
        return 0;
    }
    Node* list = readFile(fd);
    if(isInts){
        sortType ? insertionSort(&list, *intCompare) : quickSort(&list, *intCompare);
    
    }
    else{
        sortType ? insertionSort(&list, *stringCompare) : quickSort(&list, *stringCompare);
    }
    Node* temp = list;
    if(temp == NULL)
        printf("Warning: file is empty\n");
    while(temp != NULL){
        if(isInts)
            printf("%d\n", *((int*)temp->value));
        else
            printf("%s\n", temp->value);
        temp=temp->next;
    }
    freeList(list);
    return 0;
}

Node* readFile(int fd){
    Node* list = NULL;
    char* word = malloc(32);
    if(word == NULL){
        printf("Fatal Error: Failed to malloc\n");
        close(fd);
        exit(0);
    }
    int buffersize = 32;
    int length = 0;
    word[0] = '\0';
    int status = 1;
    
    int size = lseek(fd, 0, SEEK_END);
    lseek(fd, 0, SEEK_SET);
    char* input = malloc(size);
    if(input == NULL){
        printf("Fatal Error: Failed to malloc\n");
        free(word);
        close(fd);
        exit(0);
    }
    int readin = 0;
    while(1){
        int status = read(fd,input + readin,size - readin);
        if(status == -1){
            printf("Fatal Error: Error number %d while reading file\n", errno);
            free(input);
            free(word);
            freeList(list);
            exit(0);
        }
        readin += status;
        if(readin == size)
            break;
    }
    close(fd);
    int i;
    for(i = 0; i < size; i++){
        if(input[i] == ','){
            Node* temp = malloc(sizeof(Node));
            if(temp == NULL){
                printf("Fatal Error: Failed to malloc\n");
                free(input);
                free(word);
                freeList(list);
                close(fd);
                exit(0);
            }
            temp->next = list;
            if(isInts){
                int* integ = malloc(sizeof(int));
                if(integ == NULL){
                    printf("Fatal Error: Failed to malloc\n");
                    free(input);
                    free(word);
                    freeList(list);
                    exit(0);
                }
                *integ = atoi(word);
                temp->value = integ;
            }
            else{
                char* repl = malloc(length + 1);
                memcpy(repl, word, length+1);
                temp->value = repl;
            }
            list = temp;
            word[0] = '\0';
            length = 0;
        }
        
        else if(input[i] == ' ' || input[i] == '\n' || input[i] == '\t'  || input[i] == '\r' || input[i] == '\v' || input[i] == '\a' || input[i] == '\b' || input[i] == '\f');
        else{
            isInts = isInts & isDigit(input[i]);
            strncat(word, input + i, 1);
            ++length;
            if((length + 1) == buffersize){
                char* repl = malloc(buffersize * 2);
                memcpy(repl, word, buffersize);
                buffersize = buffersize * 2;
                free(word);
                word = repl;
            }
        }
    }
    if(word[0] != '\0'){
        Node* temp = malloc(sizeof(Node));
        temp->next = list;
        if(isInts){
            int* integ = malloc(sizeof(int));
            *integ = atoi(word);
            temp->value = integ;
        }
        else{
            char* repl = malloc(length + 1);
            memcpy(repl, word, length + 1);
            temp->value = repl;
        }
        list = temp;
    }
    free(input);
    free(word);
    return list;
}

int intCompare(void* arg1, void* arg2){
    int a = *((int*) arg1);
    int b = *((int*) arg2);
    if(a == b){
        return 0;
    }
    return a > b ? 1 : -1;
}

int stringCompare(void* arg1, void* arg2){
    char* a = arg1;
    char* b = arg2;
    int i = 0;
    while(1){
        if(a[i] == b[i] && a[i] == '\0'){
            return 0;
        }
        if(a[i] == b[i]){
            i++;
            continue;
        }
        else{
            return a[i] > b[i] ? 1 : -1;
        }
    }
}

int insertionSort(void* toSort, int (*comparator)(void*, void*)){
    Node* head = *((Node**) toSort);
    if(head == NULL)
        return 0;
    Node* list = head->next;
    head->next = NULL;
    while(list != NULL){
        if(comparator((void*)list->value, (void*) head->value) == -1){
            Node* temp = list;
            list = list->next;
            temp->next = head;
            head = temp;
        }
        else{
            Node* leading = head->next;
            Node* trailing = head;
            while(leading != NULL && comparator((void*)list->value, (void*) leading->value) == 1){
                leading = leading->next;
                trailing = trailing->next;
            }
            Node* temp = list;
            list = list->next;
            trailing->next = temp;
            temp->next = leading;
        }
    }
    Node** root = (Node**) toSort;
    *root = head;
    return 0;
}

int quickSort(void* toSort, int(*comparator)(void*, void*)){
    Node* head = *((Node**) toSort);
    if(head == NULL)
        return 0;
    if(head->next == NULL)
        return 0;
    Node* list = head->next;
    head->next = NULL;
    Node* smaller = NULL;
    Node* larger = NULL;
    while(list != NULL){
        if(comparator((void*)list->value, (void*) head->value) == -1){
            Node* temp = list;
            list = list->next;
            temp->next = smaller;
            smaller = temp;
        }
        else{
            Node* temp = list;
            list = list-> next;
            temp->next = larger;
            larger = temp;
        }

    }
    quickSort((void*) &smaller, comparator);
    quickSort((void*) &larger, comparator);
    head->next = larger;
    Node* temp = smaller;
    if(temp != NULL){
        while(temp->next != NULL)
            temp = temp->next;
        temp->next = head;
    }
    else{
        smaller = head;
    }
    Node** root = (Node**) toSort;
    *root = smaller;
    return 0;
}
