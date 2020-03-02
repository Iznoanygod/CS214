#include <stdlib.h>
#include <stdio.h>
#include <time.h>

int main(int argc, char** argv){
    if(argc != 3){
        printf("Usage: stringGenerator file size\n");
        return 0;
    }
    char* file = argv[1];
    int size = atoi(argv[2]);
    
    FILE* fd;
    fd = fopen(file,"a+");
    char* buffer = malloc(size);
    int i;
    srand(time(0));
    for(i = 0; i < size; i++){
        int cha = rand() % 27;
        switch(cha){
            case 26:
                buffer[i] = ',';
                break;
            default:
                buffer[i] = 'a' + cha;
                break;
        }

    }
    
    fwrite(buffer, 1, size, fd);
    fclose(fd);
    return 0;
}
