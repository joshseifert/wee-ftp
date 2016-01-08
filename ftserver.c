/*
Name:	ftserver.c
Author:	Josh Seifert
Date Last Modified:	08/07/2015

Description: This is a simple ftp server, written in C, to demonstrate socket programming.
It communicates with ftclient.py, allowing the client to view the files in the server's directory,
as well as retrieve any of those files. It may be launched from the command line with one argument,
the command port number. The server is set up and runs continuously, waiting for clients to connect.
When a client connects, the server validates that it sent an appropriate command. If did not, it
returns an error message to the client. Otherwise, it establishes a data socket on the port provided
by the client, and returns the requested information. It closes the data socket after sending any 
messages, and continues listening for the next client. It does not exit until the user issues an
interrupt signal.

A high-level understanding of C programming comes from Beej's Guide to Internet Programming Using
Network Sockets, retrieved from http://beej.us/guide/bgnet/output/html/singlepage/bgnet.html. 
Additional sample code, specifically examples regarding the code in the "createSocket" function, are
based on the tutorial at http://www.linuxhowtos.org/C_C++/socket.htm

Much of this code is reused from both Project 1 from CS 372, as well as from code I originally 
wrote in the Spring 2015 term for CS 344, Operating Systems 1, Program #4.
Additional credit is attributed to the notes and lecture material by Instructor Benjamin Brewster.
Specific lines of code borrowed from other sources are given inline citations.

This program has been tested and verified to run correctly on OSU's Flip servers.
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h> 
#include <netdb.h>
#include <arpa/inet.h>
#include <dirent.h> //Used to get files in current directory

int createSocket(int port);
int connectSocket(int serverSocket);
void runServer(int serverCommandSocket);
char getClientCommand(int clientCommandSocket, int *dataPort, char *fileName);
void listFiles(int clientDataSocket, int clientCommandSocket, char *message);
void transferFile(int dataSocket, int commandSocket, char *message, char *fileName);

/*
MAIN
Receives: command socket port number, from command line
Returns: None
Function:
	The main function checks for the correct number of command line arguments,
	then assigns those arguments to variables. It launches the functions that 
	initialize and run the server. Because the server runs in an infinite loop,
	control should not be returned to main once runServer is called.
*/

int main(int argc, char **argv)
{
	if(argc != 2)
	{
		printf("Usage: ftserver portNumber\n");
		exit(1);
	}
	
	int commandPort, commandSocket;	
	
	commandPort = atoi(argv[1]);
	commandSocket = createSocket(commandPort);
	
	runServer(commandSocket);
	
	/*runServer runs in infinite loop, should not get here*/		
	return 0;
}

/*
createSocket
Receives: port, an integer value containing the port number on which the command socket is created.
Returns: sockfd, an integer value containing the socket address on which clients may connect.
Function: Creates a socket on the server and configures it. Binds it to the specific port specified
	by the user, and then sets the server up to listen. Any errors return appropriate messages.
	Used to create both the command socket and data sockets for FTP connection.
*/

int createSocket(int port)
{
	int sockfd;
	int valOne = 1;
	struct sockaddr_in server;
	
    /*Create a socket on the server*/
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		perror("socket");
		exit(1);
	}
	
	/*Configure server*/
	memset(&server, '\0', sizeof(server));
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(port);
	
	//Set socket options, so can reuse previously bound ports. This line from:
	//http://pubs.opengroup.org/onlinepubs/009695399/functions/setsockopt.html
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &valOne, sizeof(int)) == -1)
	{
		perror("setsockopt");
		exit(1); 
	}

	/*Bind socket to the port specified at the command line*/
	if(bind(sockfd, (struct sockaddr *) &server, sizeof(server)) == -1)
	{
		perror("bind call failed");
		exit(1);
	}

	/*Listen on the port for connections.*/
	if(listen(sockfd, 10) == -1)
	{
		perror("listen");
		exit(1);
	}
	
	return sockfd;
}

/*
connectSocket
Receives: serverSocket, an integer value with the value of either the server's command or data socket.
Returns: clientSocket, an integer value containing the socket address at which the server and client connect.
Function: Accepts the client's attempt to connect to the socket created by the server. This was originally
	part of the createSocket function, but was separated because the command socket is only created on the server
	once, when it initially starts up, but is connected to the client with each new client connection. In 
	contrast, the data socket is both created and connected with each new client, so it must be separate.
*/
int connectSocket(int serverSocket)
{
	struct sockaddr_in client_addr;
	int clientSocket, addressLength;
	
	addressLength = sizeof(client_addr);

	if((clientSocket = accept(serverSocket, (struct sockaddr*) &client_addr, &addressLength)) == -1)
	{
		perror("Error accepting the client connection, please try again.\n");
		return -1;
	}
	else
	{
		return clientSocket;
	}
}
/*
RUNSERVER
Receives: serverCommandSocket, the file descriptor for the server's command socket 
Returns: None
Function: Runs in an infinite loop, accepting connections from ftp clients.
	When a connection is made, calls a function to parse client's command parameters.
	Validates that it received a valid command, and launches functions to either
	send the current directory, or requested file to the client. Once done, closes 
	sockets and returns to the top of infinite loop, so that it may receive another
	client connection. 	
*/
void runServer(int serverCommandSocket)
{
	printf("Server is now running.\n");
	
	int clientCommandSocket, serverDataSocket, clientDataSocket, dataPort;	
	char clientCommand, fileName[512], message[1024];
	
	/*Run server in infinite loop, returns here when client disconnects*/
	while(1)
	{
		printf("Waiting for client connections...\n\n");
		
		if((clientCommandSocket = connectSocket(serverCommandSocket)) == -1)
		{
			perror("Error connecting to the client's command socket. Aborting...\n\n");
			break;
		}
		printf("Successfully connected to the client!\n\n");
		
		//Should be "l" or "r", also gets values for dataPort and fileName
		clientCommand = getClientCommand(clientCommandSocket, &dataPort, fileName);
		
		//Create a data socket to transmit information
		serverDataSocket = createSocket(dataPort);
		clientDataSocket = connectSocket(serverDataSocket);	

		if(clientCommand == 'l')
		{
			listFiles(clientDataSocket, clientCommandSocket, message);
		}
		else if(clientCommand == 'g')
		{
			transferFile(clientDataSocket, clientCommandSocket, message, fileName);
		}
		//else client sent invalid command, return error message
		else
		{
			printf("Client sent unexpected command \"-%c\", returning error message to client.\n",clientCommand);
			//send error message on command socket, not data socket
			strcpy(message, "Invalid command. Use \"-l\" to list files, or \"-g <filename>\" to retrieve a file.\n");
			if(write(clientCommandSocket,message,strlen(message)) == -1)
			{
				perror("Error writing to client.\n");
			}
		}
		close(serverDataSocket);
		close(clientDataSocket);
		close(clientCommandSocket);
	}
}

/*
getClientCommand
Receives: clientCommandSocket, the file descriptor for the client's command socket,
		*dataPort, a pointer that saves the port number on which to establish the data socket,
		*fileName, a pointer to an array that saves the name of the file to be retrieved.
Returns: a character corresponding to the requested command.
Function: Reads values from the client, via the command socket. Tokenizes the input into
		the client's command, the data port, and the file name. Returns just the character
		(not the entire command string) of the requested action.
*/

char getClientCommand(int clientCommandSocket, int *dataPort, char *fileName)
{
	char buffer[512];
	char cmd[3];
	
	memset(buffer, '\0', 512);
	memset(cmd, '\0', 3);
	memset(fileName, '\0', 512);	
	
	if(read(clientCommandSocket, buffer, sizeof(buffer)) == -1)
	{
		perror("read error");
		exit(1);
	}
	else
	{
		//printf("\nDEBUG: buffer is %s\n",buffer);
		
		char *token;
		
		token = strtok(buffer, " \n\0");
		strncpy(cmd, token, 3);
		
		token = strtok(NULL, " \n\0");
		*dataPort = atoi(token);
		
		token = strtok(NULL, " \n\0");
		strncpy(fileName, token, 512);
				
		return cmd[1];	//should be 'l' to list, 'g' to retrieve.
	}
}

/*
listFiles
Receives: clientDataSocket, the file descriptor for the client's data socket
		clientCommandSocket, the file descriptor for the client's command socket,
		*message, a buffer to write messages and file directory to the client.
Returns: none
Function: Write a message to the client informing them that data on the data socket is incoming.
		Opens the current directory, writes all of the files to an array, and then sends that
		array to the client via the data socket.
*/

void listFiles(int clientDataSocket, int clientCommandSocket, char *message)
{
	printf("Sending current file directory to the client...\n");
	
	memset(message, '\0', 1024);
	
	//Client expects a return message on the control socket, either confirming that the operation
	//is proceeding, or an error message.
	strcpy(message, "VALID_COMMAND");
	if(write(clientCommandSocket,message,strlen(message)) == -1)
	{
		perror("Error writing to client.\n");
	}
	
	memset(message, '\0', 1024);
	
	//The following few lines of code, up through closing the directory, borrowed from one of the answers provided at:
	//http://stackoverflow.com/questions/4204666/how-to-list-files-in-a-directory-in-a-c-program
	DIR *d;
	struct dirent *dir;
	//Open the current directory
	d = opendir(".");
	if (d)
	{
		while ((dir = readdir(d)) != NULL)
		{
			//Record the name of each file in the directory
			strcat(message, dir->d_name);
			strcat(message, "\n");			
		}
		closedir(d);
		/*If no files in current directory, send message indicating empty directory.
		In the case of this program, this will never occur, since the server always returns the current
		directory which has, at least, the executable ftserver file itself.*/
		if(strlen(message) <= 1)
		{
			strcpy(message, "The directory is empty.");
		}
		//Send directory to client via the Data socket.
		if(write(clientDataSocket,message,strlen(message)) == -1)
		{
			perror("Error writing to client.\n");
		}		
	}
	printf("done.\n");
}

/*
transferFile
Receives: clientDataSocket, the file descriptor for the client's data socket
		clientCommandSocket, the file descriptor for the client's command socket,
		*message, a buffer to write messages and file directory to the client,
		*fileName, a pointer to an array with the name of the file to be retrieved.
Returns: none
Function: Write a message to the client informing them that data on the data socket is incoming.
		Opens the requested file in the current directory, if available, else sends an error message
		to the client. Reads data from the open file into a buffer, then sends that data via the Data
		Socket, until EOF is reached.
*/
void transferFile(int clientDataSocket, int clientCommandSocket, char *message, char *fileName)
{
	printf("Sending requested file \"%s\" to the client...\n",fileName);
	
	memset(message, '\0', 1024);

	FILE *filefd;
	int numBytes;
	char dataBuffer[512];
	
	//open file as read only
	if((filefd = fopen(fileName, "r")) == NULL)
	{
		//If unable to open file, send message on Command socket.
		strcpy(message,"Error: Requested file not found in the current directory. Aborting...\n");
		printf("%s",message);
		if(write(clientCommandSocket,message,strlen(message)) == -1)
		{
			perror("Error writing to client.\n");
		}
		return;		
	}
	//Client expects a return message on the Command socket, either confirming that the operation
	//is proceeding, or an error message.
	else
	{
		strcpy(message, "VALID_COMMAND");
		if(write(clientCommandSocket,message,strlen(message)) == -1)
		{
			perror("Error writing to client.\n");
		}		
	}
	
	memset(message, '\0', 1024);
	
	//Large files will not fit in one data packet, so send small packets until fread returns 0, indicating EOF.
	do{
		if((numBytes = fread(dataBuffer,1,512,filefd)) == -1)
		{
			perror("Error reading from file.\n");
		}		
		//printf("DEBUG: read the following string from file:\n%s",dataBuffer);
		if(write(clientDataSocket, dataBuffer, numBytes) == -1)
		{
			perror("Error writing to client.\n");
		}
	}while(numBytes > 0);

	fclose(filefd);	
	
	printf("done.\n");
}