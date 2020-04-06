#include <stdio.h>
#include <stdlib.h>
#include "fileCompressor.h"
#include "heapSort.h"

void heapify(Node*** arr, int n, int i){
    Node** array = *arr;
    int largest = i; 
    int l = 2*i + 1;
    int r = 2*i + 2; 
  
    if (l < n && array[l]->frequency > array[largest]->frequency) 
        largest = l; 
  
    if (r < n && array[r]->frequency > array[largest]->frequency) 
        largest = r; 
    if (largest != i) 
    { 
        Node* temp = array[i];
        array[i] = array[largest];
        array[largest] = temp;
  
        heapify(&array, n, largest); 
    } 
    *arr = array;
}

void heapSort(Node** arr, int size){
    int i;
    Node** array = arr;
    for(i = size / 2 - 1; i >= 0; --i)
        heapify(&array, size, i);
    int j;
    for (j = size-1; j>0; j--){
        Node* temp = array[0];
        array[0] = array[j];
        array[j] = temp;

        heapify(&array, j, 0);
    }
}
