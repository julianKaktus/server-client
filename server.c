#include <stdio.h>
#include <stdlib.h>

#include <netdb.h>
#include <pthread.h>

#include <string.h>

#define COMMAND_LENGTH 255
#define SERVER_PORT 1417

int main (int argc, char *argv[])
{
	int serverSocket = initServer();
  	// Listening for clients

	struct sockaddr_in cli_addr;
	listen(serverSocket, 5);
	int cliLen = sizeof(cli_addr);
	
	// ToDo: Add Threads!
	while(1)
	{
		printf("Waiting for connections...\n");

		// Accept actual connection from the client
		int clientSocket = accept(serverSocket, (struct sockaddr *) &cli_addr, &cliLen);
		if (clientSocket < 0)
		{
			perror("Error on accept\n");
			exit(1);
		}
		printf("Client connected: %d \n", cli_addr.sin_addr.s_addr);
		// Start communicating
		char command[COMMAND_LENGTH];
		int n = read(clientSocket, command, sizeof(command));

		if(n<0)
		{
			perror("Error reading from socket\n");
			exit(1);
		}

		printf("Here is the message: %s \n", command);

		shutdown(clientSocket, SHUT_RDWR);
		close(clientSocket);
		printf("Connection to client closed.");
	}

	return 0;

}

int initServer()
{
	int serverSocket = socket(AF_INET, SOCK_STREAM, 0);

	if (serverSocket < 0)
	{
		perror("Error opening socket\n");
		exit(1);
	}
	printf("Socket opened.\n");

  // Initialize socket structure
	struct sockaddr_in serv_addr;

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(SERVER_PORT);

  // Bind host adress
	if(bind(serverSocket, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
	{
		perror("Error on binding\n");
		exit(1);
	}

	return serverSocket;
}
