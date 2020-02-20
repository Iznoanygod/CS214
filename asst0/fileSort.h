#ifndef _FILESORT_H
#define _FILESORT_H 1
int insertionSort(void* toSort, int (*comparator)(void*, void*));
int quickSort(void* toSort, int (*comparator)(void*, void*));
int stringCompare(void* arg1, void* arg2);
int intCompare(void* arg1, void* arg2);
int isInts = 1;
typedef struct Node{
    void* value;
    struct Node* next;
}Node;
Node* readFile(int fd);
void freeList(void* arg){
    Node* leading = (Node*) arg;
    while(1){
        if(leading == NULL)
            return;
        Node* trailing = leading;
        leading = leading->next;
        free(trailing->value);
        free(trailing);
    }
}
int isDigit(char a){
    return a > 47 && a < 58 ? 1 : 0;
}
#endif
