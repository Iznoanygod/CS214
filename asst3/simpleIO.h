#ifndef _FILEIO_H
#define _FILEIO_H 1
typedef struct FNode
{
    char* path;
    char* hash;
    int version;
    struct FNode* next;
}FNode;
int insertionSort(FNode** toSort);
int simpleRead(int fd, char* buffer, int maxRead);
int simpleWrite(int fd, char* buffer, int maxWrite);
int unTar(char* path, char* loc);
int Tar(char* path, char* loc);
char* md5(char* path);
#endif
