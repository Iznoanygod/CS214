#ifndef _FILECOMPRESSOR_H
#define _FILECOMPRESSOR_H 1
typedef struct Node{
    char* value;
    int frequency;
    struct Node* left;
    struct Node* right;
}Node;
typedef struct File{
    char* path;
    int fd;
    struct File* next;
}File;
typedef struct cbLL{
    char* code;
    char* token;
    struct cbLL* next;
}cbLL;
Node* tokenizeDict(int fd);
int readFile(int fd, Node*** arr, int size);
char* stringToken(char* token);
void decompressFile(Node* tree, int ofd, int nfd);
void compressFile(cbLL* codes, int ofd, int nfd);
void createDictionary(Node* tree, int fd);
File* recurseFiles(char* path);
void freeTree(void* root);
Node* createTree(Node** arr, int size);
void sortArray(Node*** arr, int size);
#endif
