#ifndef _FILECOMPRESSOR_H
#define _FILECOMPRESSOR_H 1
typedef struct Node{
    void* value;
    int frequency;
    struct Node* left;
    struct Node* right;
}Node;
Node* tokenizeDict(int fd);
void decompressFile(Node* tree, int ofd, int nfd);
void createDictionary(Node* tree, int fd);
void recurseCreate(Node* tree, int fd, char* path);
void freeTree(void* root);
#endif
