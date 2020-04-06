#ifndef _FILECOMPRESSOR_H
#define _FILECOMPRESSOR_H 1
typedef struct Node{
    char* value;
    int frequency;
    struct Node* left;
    struct Node* right;
}Node;

/** Add these **/
/******************************************************************/
typedef struct LLNode{
	struct LLNode* next;
	char* value;
}LLNode;
typedef struct DictNode{
	struct DictNode* next;
	char* value;
	char* key;
}DictNode;

void freeLL(LLNode* head);
void freeDictLL(DictNode* head);
int isDelimiter(char c);
LLNode* getTokens(char* buf, int bufSize, int* offset);
char* lookupHuffmanCode(char* token, DictNode* dictHead);
DictNode* readCodebook(int codebookFD);
void compress(char* compressPath, int codebookFD);
/******************************************************************/



Node* tokenizeDict(int fd);
int readFile(int fd, Node*** arr, int size);
char* stringToken(char* token);
void decompressFile(Node* tree, int ofd, int nfd);
void createDictionary(Node* tree, int fd);
void freeTree(void* root);
Node* createTree(Node** arr, int size);
#endif
