#include <stdlib.h>
#include <stdio.h>
#include "fileCompressor.h"
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>


/**
* Compresses the specified file using the specified codebook.
* @param compressPath	Path of file to compress
* @param codebookFD		File descriptor of codebook
* @return 
**/
void compress(char* compressPath, int codebookFD)
{
	int fd = open(compressPath, O_RDWR);
	if(fd == -1)
	{
		switch(errno)
		{
			case ENOENT:
				printf("Fatal Error: Specified file does not exist\n");
				break;
			case EISDIR:
				printf("Fatal Error: File path given is a directory\n");
				break;
			default:
				printf("Fatal Error: Unspecified error occurred while opening file\n");
				break;
		}
		exit(errno);
	}

	int compressFD = open(strcat(compressPath, ".hcz"), O_RDWR | O_CREAT);
	DictNode* codebookHead = readCodebook(codebookFD);
	
	char inbuf[256];
	int bytesRead;
	do
	{
        bytesRead = read(fd, inbuf, 256);
        if(bytesRead == -1)
		{
            printf("Fatal Error: Error while reading file\n");
            exit(errno);
        }
		if (bytesRead == 0)
		{
			break;
		}
		
		int offset;
		LLNode* head = getTokens(inbuf, bytesRead, &offset);
		lseek(fd, -offset, SEEK_CUR);
		
		
		LLNode* curr = head;
		int totalBytesWrote = 0;
		int bytesWrote;
		while (curr != NULL)
		{
			char* code = lookupHuffmanCode(curr->value, codebookHead);
			
			// TODO:
			// X1) huffman codebook should be read in as a dictionary, key and value
			// X2) LLNode should be \0 terminated, so we can use strcmp
			// X3) lseek instead of decrement totalBytesRead
			// X4) fix delimiters in linked list (should be double escaped)
			// X5) free linked list mem function
			// 6) Make sure compressFD does not get reset back to beginning (shouldnt)
			
			while (totalBytesWrote < strlen(curr->value))
			{
				bytesWrote = write(compressFD, code+totalBytesWrote, strlen(curr->value)-totalBytesWrote);
				totalBytesWrote+= bytesWrote;
			}
			totalBytesWrote = 0;
			
			curr=curr->next;
		}
		freeLL(head);
	}while(bytesRead != 0);
	
	freeDictLL(codebookHead);
	close(fd);
	close(compressFD);
}

/**
* Reads the specified codebook into a dictionary linked list
* @param codebookFD		File descriptor of code book
* @return head			Head node of codebook dictionary
**/
DictNode* readCodebook(int codebookFD)
{
	int fsize = lseek(codebookFD, 0, SEEK_END);
    lseek(codebookFD, 0, SEEK_SET);
	
	DictNode* head = malloc(sizeof(DictNode));
	DictNode* curr = head;
	
	char buf[fsize];
	int totalBytesRead = 0;
	int bytesRead;
	do
	{
		bytesRead = read(codebookFD, buf+totalBytesRead, fsize-totalBytesRead);
		totalBytesRead+=bytesRead;
	}while (bytesRead != 0);
	
	int i, mark = 0;
	// fsize-1 bc we are going to ignore the last extra blank line
	for (i = 0; i < fsize-1; i++)
	{
		if (buf[i] == '\n')
		{
			int subbufSize = i-mark;
			char subbuf[subbufSize];
			memcpy(subbuf, &buf[mark], subbufSize);
			
			int j;
			for (j = 0; j < subbufSize; j++)
			{
				if (subbuf[j] == '\t')
				{
					char valbuf[j];
					memcpy(valbuf, &subbuf[0], j-1);
					valbuf[j-1] = '\0';
					
					char keybuf[subbufSize-j];
					memcpy(keybuf, &subbuf[j+1], subbufSize-j-1);
					keybuf[subbufSize-j-1] = '\0';
					
					curr->key = keybuf;
					curr->value = valbuf;
					curr->next = malloc(sizeof(DictNode));
					curr = curr->next;
					break;
				}
			}
			mark = i+1;
		}
	}
	
	return head;
}

/**
* Looks up huffman code for specified token in specified dictionary
* @param token		Token to look up
* @param dictHead		Head node of dictionary linked list containing the codebook
* @return huffmanCode		huffman code for specified token in specified dictionary
**/
char* lookupHuffmanCode(char* token, DictNode* dictHead)
{
	DictNode* curr = dictHead;
	while (curr != NULL)
	{
		if (!strcmp(curr->key, token))
		{
			return curr->value;
		}
		curr = curr->next;
	}
	
	// TODO
	// This means that we dont have a code for the token, which means the user most likely specified the wrong codebook
	// We need to print out an error or something for this
	return -1;
}

/**
* Gets tokens in a chunk of memory and returns them in a linked list
* NOTE: We are assuming each word is less than or equal to 256 characters
* @param buf		Char buffer of memory to look through
* @param bufSize	Size of char buffer
* @param offset		Number of bytes to move backwards (we want to lseek back to the end of last token, just in case we cut off a token)
* @return head		Head of linked list of tokens
**/
LLNode* getTokens(char* buf, int bufSize, int* offset)
{
	LLNode* head = malloc(sizeof(LLNode));
	LLNode* curr = head;
	
	int i;
	int mark = 0;
	int delimCount = 0;
	for (i = 0; i < bufSize; i++)
	{
		if (isDelimiter(buf[i]))
		{
			// Add word to linked list
			char subbuf[i-mark+1];
			memcpy(subbuf, &buf[mark], i-mark);
			subbuf[i-mark] = '\0';
			
			curr->value = subbuf;
			curr->next = malloc(sizeof(LLNode));
			curr=curr->next;
			
			// Add delimiter to linked list
			if (buf[i] == ' ')
			{
				char delimSubBuf[2];
				memcpy(delimSubBuf, &buf[i], 1);
				delimSubBuf[1]='\0';
				curr->value = delimSubBuf;
			}
			else
			{
				char delimSubBuf[3];
				delimSubBuf[0] = '\\';
				memcpy(delimSubBuf, &buf[i]+1, 1);
				delimSubBuf[2]='\0';
				curr->value = delimSubBuf;
			}
				
			curr->next = malloc(sizeof(LLNode));
			curr = curr->next;
			
			mark = i+1;
			delimCount++;
		}
	}
	
	// if we have reached end of buffer and delimCount is 0, the entire buffer is a token (i.e. the last token)
	if (delimCount == 0)
	{
		char subbuf[bufSize];
		memcpy(subbuf, &buf[0], bufSize);
		subbuf[bufSize]='\0';
		
		curr->value = subbuf;
		curr->next = malloc(sizeof(LLNode));
		curr=curr->next;
		
		mark = bufSize;
	}
	
	*offset = bufSize - mark;
	return head;
}

/**
* Checks if the specified char is a delimiter (as specified in assignment instructions)
* @param c		Char to check
* @return		1 if c is a delimiter, 0 if c is not a delimiter
**/
int isDelimiter(char c)
{
	return (c == '\n' || c == '\t' || c == ' ' || c == '\r' || c == '\v' || c == '\f');
}

/**
* Frees linked list
**/
void freeLL(LLNode* head)
{
	while (head != NULL)
	{
		LLNode* temp = head;
		head = head->next;
		free(temp);
	}
}

/**
* Frees dictionary linked list
**/
void freeDictLL(DictNode* head)
{
	while (head != NULL)
	{
		DictNode* temp = head;
		head = head->next;
		free(temp);
	}
}
