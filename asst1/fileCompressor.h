#ifndef _FILECOMPRESSOR_H
#define _FILECOMPRESSOR_H 1
typedef struct Node{
    char* value;
    int frequency;
    struct Node* left;
    struct Node* right;
}Node;
Node* tokenizeDict(int fd);
int readFile(int fd, Node*** arr, int size);
char* stringToken(char* token);
void decompressFile(Node* tree, int ofd, int nfd);
void createDictionary(Node* tree, int fd);
void freeTree(void* root);
Node* createTree(Node** arr, int size);
#endif
