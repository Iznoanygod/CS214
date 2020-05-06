#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

typedef struct CNode
{
	char *command;
	char *output;
	struct CNode *next;
} CNode;


void printCNodeCommands(CNode *head)
{
	CNode *curr = head;
	while (curr != NULL)
	{
		printf("%s\n", curr->command);
		curr = curr->next;
	}
}

void printCNodeOutputs(CNode *head)
{
	CNode *curr = head;
	while (curr != NULL)
	{
		printf("%s\n", curr->output);
		curr = curr->next;
	}
}

void freeCNodeList(CNode *head)
{
	CNode *curr = head;
	while (curr != NULL)
	{
		free(curr->command);
		free(curr->output);
		CNode *temp = curr;
		curr = curr->next;
		free(temp);
	}
}



int main(int argc, char **argv)
{
	sleep(5);
	int fd = open(argv[1], O_RDONLY);
	int size = lseek(fd, 0, SEEK_END);
	lseek(fd, 0, SEEK_SET);
	
	char buff[size];
	int totalBytesRead = 0;
	while (totalBytesRead < size)
	{
		int bytesRead = read(fd, buff+totalBytesRead, size-totalBytesRead);
		totalBytesRead += bytesRead;
	}
	
	CNode *head = malloc(sizeof(CNode));
	CNode *curr = head;
	int i, mark = 0;
	for (i = 0; i < size; i++)
	{
		if (buff[i] == '\t')
		{
			char *command = malloc(i - mark +1);
			memcpy(command, &buff[mark], i-mark);
			command[i-mark] = '\0';
			curr->command = command;
			mark = i+1;
		}
		if (buff[i] == '\n')
		{
			char *output = malloc(i - mark+1);
			memcpy(output, &buff[mark], i-mark);
			output[i-mark] = '\0'; 
			//printf("%i\n", (int)output[i-mark]);
			curr->output = output;
			mark = i+1;
			if (i!=size-1)
			{
				curr->next = malloc(sizeof(CNode));
				curr = curr->next;
			}
		}
	}
	
	close(fd);
	curr = head;
	while (curr != NULL)
	{
		char sysCom[128];
		sysCom[0] = '\0';
		strcat(sysCom,curr->command);
		strcat(sysCom," > ");
		strcat(sysCom, argv[2]);
		system(sysCom);
		
		int tempoutfd = open(argv[2], O_RDONLY);
		int size2 = lseek(tempoutfd, 0, SEEK_END);
		lseek(tempoutfd, 0, SEEK_SET);
		char buff2[size2+1];
		int totalBytesRead2 = 0;
		
		while (totalBytesRead2 < size2)
		{
			int bytesRead = read(tempoutfd, buff2+totalBytesRead2, size2-totalBytesRead2);
			totalBytesRead2 += bytesRead;
		}
		
		if (!strcmp(curr->output, ""))
		{
			buff2[size2] = '\0';
			char *result = (!strcmp(buff2, curr->output)) ? "\033[0;32m[PASSED]\033[0m" : "\033[0;31m[FAILED]\033[0m";
			printf("%s\t%s\n", curr->command, result);
			if (strcmp(buff2, curr->output))
			{
				printf("\n\n%s\n%s\n\n", buff2, curr->output);
			}
		}
		else if (!strcmp(curr->output, "#"))
		{
			printf("%s\n", curr->command);
		}
		else if (!strcmp(curr->output, "*"))
		{
			buff2[size2] = '\0';
			printf("\n%s\t%s\n--------------------\n", curr->command, "\033[0;33m[YOU DECIDE]\033[0m");
			printf("%s\n--------------------\n\n", buff2);
		}
		else
		{
			mark = 0;
			int lastmark = mark;
			for (i = 0; i < size2; i++)
			{
				if (buff2[i] == '\n')
				{
					lastmark = mark;
					mark = i+1;
				}
			}
			char realOutput[size2-lastmark];
			memcpy(realOutput, buff2+lastmark, size2-lastmark-1);
			realOutput[size2-lastmark-1] = '\0';
			
			//printf("%s, %s\n", curr->command, curr->output);
			//printf("%s\n%s\n\n", curr->output, realOutput);
			char *result = (!strcmp(realOutput, curr->output)) ? "\033[0;32m[PASSED]\033[0m" : "\033[0;31m[FAILED]\033[0m";
			printf("%s\t%s\n", curr->command, result);
			if (strcmp(realOutput, curr->output))
			{
				//printf("%s\n", buff2);
				printf("\n!!!!!!!!!!!!!!!!!!!!!!!!!!\nExpected: %s\nBut got: %s\n!!!!!!!!!!!!!!!!!!!!!!!!!!\n\n",curr->output, realOutput);
			}
		}
		curr = curr->next;
		close(tempoutfd);
		sleep(0.75);
	}
	
	//printCNodeCommands(head);
	//printCNodeOutputs(head);
	remove(argv[2]);	
	freeCNodeList(head);
}
