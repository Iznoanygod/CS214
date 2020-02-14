#ifndef _FILESORT_H
#define _FILESORT_H
int insertionSort(void* toSort, int (*comparator)(void*, void*));
int quicksort(void* toSort, int (*comparator)(void*, void*));
int stringCompare(char* arg1, char* arg2);
int intCompare(int arg1, int arg2);
typedef struct Node{
    void* value;
    struct Node* next;
}Node;
#endif
