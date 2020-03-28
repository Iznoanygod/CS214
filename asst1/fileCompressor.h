#ifndef _FILECOMPRESSOR_H
#define _FILECOMPRESSOR_H 1
typedef struct TreeNode{
    void* value;
    int frequency;
    struct TreeNode* left;
    struct TreeNode* right;
}TreeNode;
void tokenizeDict(int fd);
void freeTree(void* root);
#endif
