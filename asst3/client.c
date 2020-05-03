#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <ctype.h>
#include "client.h"
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <signal.h>
#include <math.h>

#define BUFF_SIZE 1024
#define DELIM ":"


/** TODO
* - function that builds a file/directory structure from a .manifest file
* - function that updates a file/directory structure from a .manifest file
* - function that compares client and server .manifest files and outputs changes to .Update
*		- cross check with a list of files that the user has updated (currently working on), write to .Conflict when necessary
* - function that gets manifest version
**/

int sock;

int main(int argc, char** argv){
    signal(SIGINT, sig_handler);
	
	char message[BUFF_SIZE];
	char ip[64];
	char port[6];
	
	if(argc == 1)
	{
        printf("Fatal error: no argument given\n");
        exit(EXIT_FAILURE);
    }
	
    if(!strcmp(argv[1], "configure"))
	{
        if(argc != 4)
		{
            printf("Fatal error: wrong number of arguments given\n");
            exit(EXIT_FAILURE);
        }
        if(!isNumber(argv[3]))
		{
            printf("Fatal error: port number is not valid\n");
            exit(EXIT_FAILURE);
        }
        writeConfig(argv[2], argv[3]);
    }
	if (!strcmp(argv[1], "checkout"))
	{
	
		
		// check if project name exists on client side
		// check if project exists on server
		// send manifest
		// recv new manifest
		// construct directories from new manifest
		// begin receiving files
	}
	if (!strcmp(argv[1], "update"))
	{
		
		
		// check if project name exists on server
		// check if our project is up to date (send manifest to server, server compares and responds)
		// recv new manifest
		// compare new and old manifests
		// follow more in depth in the assignment instructions
	}
	if (!strcmp(argv[1], "upgrade"))
	{
		
		
		// check if there is a .Update file		yes-->proceed, no-->tell user to run update
		// if .Update empty tell user we up to date
		// check if there is a .Conflict file   no-->proceed, yes-->tell user to resolve conflicts and update
		// check if project name exists on server
		// delete all files tagged with D
		// fetch files from server marked with M or A
		// delete .Update file
	}
	if (!strcmp(argv[1], "commit"))
	{
		
		
		// check if nonempty .Update file exists	no-->proceed
		// check if .Conflict file exists			no-->proceed
		// check if project name exists on server
		// check if our project is up to date (send manifest version to server, server compares and responds)  no-->tell user to run update
		// go through every file listed in manifest and compute a live hash for it
		// compare each live hash to the stored hash in the manifest file (write each different case to a .Commit file)
		// compare servers manifest with client's, check for different hashes with >= version numbers		yes-->tell user to update&upgrade
		
	}
	if (!strcmp(argv[1], "push"))
	{
		
		
		
	}
	if (!strcmp(argv[1], "create"))
	{
		readConfig(ip, port);
		snprintf(message, BUFF_SIZE, "create:%li:%s:", strlen(argv[2]), argv[2]);
		sock = getSocket(ip, port);
		send(sock, message, strlen(message), 0);
	}
	if (!strcmp(argv[1], "destroy"))
	{
		
		
	}
	if (!strcmp(argv[1], "add"))
	{
		
		
	}
	if (!strcmp(argv[1], "remove"))
	{
		
		
	}
	if (!strcmp(argv[1], "currentversion"))
	{
		
		
	}
	if (!strcmp(argv[1], "history"))
	{
		
		
	}
	if (!strcmp(argv[1], "rollback"))
	{
		
		
	}
	if (!strcmp(argv[1], "checkout"))
	{
		
		
	}
}


int getSocket(char *ip, char *port_s)
{
	printf("%s:%s\n", ip, port_s);
	int port = atoi(port_s);
	struct sockaddr_in serv_addr;
	
	int sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0)
	{
		printf("Could not create socket.\n");
		exit(EXIT_FAILURE);
	}
	serv_addr.sin_family = AF_INET; 
    serv_addr.sin_port = htons(port);

	if (inet_pton(AF_INET, ip, &serv_addr.sin_addr) <= 0)
	{
		printf("Invalid address.\n");
		exit(EXIT_FAILURE);
	}
	
	if (connect(sock, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0)
	{
		printf("Failed to connect.\n");
		exit(EXIT_FAILURE);
	}
	
	return sock;
}

void sig_handler(int sig)
{
	printf("\nStopping client...\n");
	close(sock);
	exit(0);
}

/**
* ret[0] = ip,  ret[1] = port
**/
void readConfig(char ip[], char port[])
{
	int fd = open (".configure", O_RDONLY);
	if(fd == -1)
	{
		printf("Fatal error: error reading .configure file\n");
		exit(EXIT_FAILURE);
	}
	
	char buf[64];
	int totalBytesRead = 0;
	int bytesRead;
	do
	{
		bytesRead = read(fd, buf+totalBytesRead, 64);
		if (bytesRead == -1)
		{
			printf("Fatal error: error reading .configure file\n");
			close(fd);
			exit(EXIT_FAILURE);
		}
		totalBytesRead+=bytesRead;
	} while(bytesRead != 0);
	
	buf[totalBytesRead] = '\0';
	char *token = strtok(buf, " ");
	int i;
	for (i = 0; i < 2 && token!=NULL; i++)
	{
		//ret[i] = token;
		if (i==0) strcpy(ip,token);
		if (i==1) strcpy(port,token);
		token = strtok(NULL, " ");
	}
}

void writeConfig(char *host, char *port)
{
	int fd = open(".configure", O_RDWR | O_CREAT | O_TRUNC, S_IRWXU);
	if(fd == -1){
		printf("Fatal error: error writing to .configure file\n");
		exit(EXIT_FAILURE);
	}
	int check = write(fd, host, strlen(host));
	if(check == -1){
		printf("Fatal error: error writing to .configure file\n");
		close(fd);
		exit(EXIT_FAILURE);
	}
	check = write(fd, " ", 1);
	if(check == -1){
		printf("Fatal error: error writing to .configure file\n");
		close(fd);
		exit(EXIT_FAILURE);
	}
	check = write(fd, port, strlen(port));
	if(check == -1){
		printf("Fatal error: error writing to .configure file\n");
		close(fd);
		exit(EXIT_FAILURE);
	}
}

char * itoa(int i)
{
	int numChars = floor(1 + log10(i));
	char *ret = (char *) malloc(numChars);
	snprintf(ret, numChars+1, "%d", i);
	return ret;
}

bool isNumber(char* in){
    int i;
    for(i = 0; in[i] != '\0'; i++){
        if(!isdigit(in[i]))
            return false;
    }
    return true;
}
