#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>

#define BUFF_SIZE 64

typedef struct Node{
    void* value;
    struct Node* next;
}Node;

/**
Grabs 1 word from a char buffer, "in", using \n as the delimiter, and place it in Node ret
**/
void grabWord(char* in, Node* ret)
{
	int i;
	int last = -1;
	for (i = 0; i < BUFF_SIZE; i++)
	{
		if (in[i] == '\n')
		{
			if (last == -1)
			{
				last = i+1;
			}
			else
			{
				int sub_size = i - last + 1;
				ret->value = malloc(sub_size*sizeof(char));
				memcpy(ret->value, &in[last], (sub_size-1)*sizeof(char));
				char* temp = (char*) ret->value;
				temp[sub_size-1] = '\0';
				
				// to lower case
				int j;
				for (j = 0; j < sub_size-1; j++)
				{
					temp[j] = tolower(temp[j]);
				}
				
				ret->value = temp;
				break;
			}
		}
	}
}

void writeStringFile(char *path, Node *head)
{
	FILE *fp = fopen(path, "w+");
	Node* curr = head;
	
	char* seps[] = {" ", "\n", "\t", "\r", "\v"};
	while (curr->next != NULL)
	{
		fprintf(fp, "%s%s,", seps[rand()%9], curr->value);
		curr = curr->next;
	}
	fclose(fp);
}
void writeIntFile(char *path, Node *head)
{
	FILE *fp = fopen(path, "w+");
	Node* curr = head;
	
	char* seps[] = {" ", "\n", "\t", "\r", "\v"};
	while (curr->next != NULL)
	{
		fprintf(fp, "%s%d,", seps[rand()%9], curr->value);
		curr = curr->next;
	}
	fclose(fp);
}

int main(int argc, char** argv)
{
	
	if (strcmp(argv[1], "-h")==0)
	{
		printf("\ntestgenerator [arg] [numtokens] [filename]\n\n-i\tgenerates integer test file\n-s\tgenerates string test file\n\n");
		return 0;
	}
	// argv 1 is generate type, argv 2 is number of tokens, argv 3 is output name
	time_t t;
	srand((unsigned) time(&t));
	
	int numTokens = atoi(argv[2]);
	
	if (strcmp(argv[1], "-i") == 0)
	{
		Node* finalHead = malloc(sizeof(Node));
		Node* current = finalHead;
		int i;
		for (i = 0; i < numTokens; i++)
		{
			int nextNum = (rand()%(RAND_MAX)) - (RAND_MAX/2);
			current->value = nextNum;
	
			Node* next = malloc(sizeof(Node));
			current->next = next;
			current = next;
		}
		
		writeIntFile(argv[3], finalHead);
	}
	else if (strcmp(argv[1], "-s") == 0)
	{
		const char* dict_path = "/usr/share/dict/words";
		int fd = open(dict_path ,O_NONBLOCK, O_RDONLY);
		if(fd == -1)
		{
			printf("Fatal Error: file \"%s\" does not exist\n", dict_path);
			exit(errno);
		}
		
		char* in = malloc(BUFF_SIZE);
		unsigned int fsize = (int) lseek(fd, 0, SEEK_END);
		Node* finalHead = malloc(sizeof(Node));
		Node* current = finalHead;
		
		/**
		We are going to read in BUFF_SIZE bytes starting at a random byte location in the file,
		parse out 1 word, then repeat numTokens times. Yes, it is possible we encounter words that
		are larger than BUFF_SIZE, but this is unlikely.
		**/
		int i;
		int status = 0;
		int bytesRead = 0;
		
		for (i = 0; i < numTokens; i++)
		{
			lseek(fd, rand()%fsize, SEEK_SET);
			do
			{
				status = read(fd, in+bytesRead, BUFF_SIZE-bytesRead);
				bytesRead += status;
				if(status == -1)
				{
					printf("Fatal Error: Error number %d while reading file\n", errno);
					exit(errno);
				}
			} while (bytesRead < BUFF_SIZE);
			bytesRead = 0;
			status = 0;
			
			grabWord(in, current);
			
			Node* next = malloc(sizeof(Node));
			current->next = next;
			current = next;
			
		}
		
		writeStringFile(argv[3], finalHead);
	}
	
	return 0;
}