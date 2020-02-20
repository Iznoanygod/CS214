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
    Node* b = malloc(sizeof(Node));
    Node* c = malloc(sizeof(Node));
    Node* d = malloc(sizeof(Node));
    a->value = (void*)23;
    b->value = (void*)12;
    c->value = (void*)22;
    d->value = (void*)1;
    a->next = b;
    b->next = c;
    c->next = d;
    d->next = NULL;
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
