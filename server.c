#include <stdio.h>
#include <stdlib.h>

#include <netdb.h>

#include <sys/types.h>
#include <sys/socket.h>

#include <pthread.h>

#include <string.h>

#define COMMAND_LENGTH 255
#define SERVER_PORT 1417
#define NR_WORKERS 2

char * queue[NR_WORKERS];

void *handleRequest(void *);
void *dispatcher(void *);
void *worker(void *);

int main (int argc, char *argv[])
{
    // Start dispatcher thread:
    pthread_t th;
    pthread_create(th, NULL, dispatcher, NULL); // letzer NUll-Wert: Übergabeparameter

	int serverSocket = initServer();
  	// Listening for clients

	struct sockaddr_in cli_addr;
	listen(serverSocket, 5);
	int cliLen = sizeof(cli_addr);

	pthread_t threads[10];

	// Wait for connections
	// add commands to queue
	// dispatcher takes command from queue and assigns it to new worker thread
	// worker thread executes command
	// write results to log
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
		printf("%d", clientSocket);

		char command[COMMAND_LENGTH];	// Puffer
        recv(socket, command, COMMAND_LENGTH, MSG_WAITALL);
        addToQueue(command)^;

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

// Thread für jeden Client
// arg: client socket
void *handleRequest(void *arg)
{
	int socket = (int)arg;
	char command[COMMAND_LENGTH];	// Puffer
	recv(socket, command, COMMAND_LENGTH, MSG_WAITALL);

	close(socket);
	printf("Message: %s\n", command);
}

void *dispatcher(void *)
{
    // take command from queue
    // start new worker thread
}

void *worker(void *arg)
{
    char * command = (char*)arg;
    // execute command
    // write result to server.log
}
