#include <stdlib.h>
#include <stdio.h>
#include "fileSort.h"

int main(int argc, char** argv){
    printf("%d\n", stringCompare(argv[1], argv[2]));
    if(argc != 3){
        printf("Fatal Error: Expected two arguments, had %d\n", (argc-1));
        return 0;
    }
    return 0;
}

int intCompare(int arg1, int arg2){
    if(arg1 == arg2){
        return 0;
    }
    return arg1 > arg2 ? 1 : -1;
}

int stringCompare(char* arg1, char* arg2){
    int i = 0;
    while(1){
        if(arg1[i] == arg2[i] && arg1[i] == '\0'){
            return 0;
        }
        if(arg1[i] == arg2[i]){
            i++;
            continue;
        }
        else{
            return arg1[i] > arg2[i] ? 1 : -1;
        }
    }
}

int insertionSort(void* toSort, int (*comparator)(void*, void*)){
    return 0;
}
