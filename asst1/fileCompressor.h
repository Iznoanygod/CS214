#ifndef _FILECOMPRESSOR_H
#define _FILECOMPRESSOR_H 1
typedef struct Node{
    void* value;
    int frequency;
    struct Node* left;
    struct Node* right;
}Node;
Node* tokenizeDict(int fd);
void freeTree(void* root);
#endif
