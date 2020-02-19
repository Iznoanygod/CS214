#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "fileSort.h"

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
    Node* a = malloc(sizeof(Node));
    a->value = (void*)1;
    Node* b = malloc(sizeof(Node));
    b->value = (void*)50;
    a->next = b;
    b->next = NULL;
    insertionSort(&a, *intCompare);

    return 0;
}

int intCompare(void* arg1, void* arg2){
    if(arg1 == arg2){
        return 0;
    }
    return arg1 > arg2 ? 1 : -1;
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
        else if(head->next == NULL){
            Node *temp = list;
            list = list->next;
            head->next = temp;
        }
        else{
            Node* leading = list->next;
            Node* trailing = list;
            
        }
    }
    Node** asd = (Node**) toSort;
    *asd = head;
    return 0;
}
int quickSort(void* toSort, int(*comparator)(void*, void*)){

    return 0;
}
