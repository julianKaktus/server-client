#include <stdio.h>
#include <stdlib.h>

#include <netdb.h>

#include <pthread.h>

#define MAX_COMMAND_LENGTH 255
#define SERVER_PORT 1417
#define NR_MAX_WORKERS 2

typedef int bool;
#define true 1
#define false 0

typedef struct Node
{
	struct Node *next;
	struct Node *prev;
	char *value;
} Node;

typedef struct
{
	struct Node* head;
	struct Node* tail;
	int counter;
} List;

List *command_queue;
pthread_t worker_threads[NR_MAX_WORKERS];
int worker_thread_counter = 0;

void enqueue(char *command);
char *dequeue();
bool isEmpty();
void * handleRequest(void *);
void *dispatcher(void *);
void *worker(void *);
void writeToLog(char *text);
char *executeCommand(char *com);

int main(int argc, char *argv[])
{
	// Init queue:
	List tmpQueue;
	command_queue = &tmpQueue;
	command_queue->head = command_queue->tail = NULL;
	command_queue->counter = 0;

	// Start dispatcher thread:
	pthread_t dispatcher_thread;
	pthread_create(&dispatcher_thread, NULL, dispatcher, NULL); // letzer NUll-Wert: Übergabeparameter

	int serverSocket = initServer();
	// Listening for clients

	struct sockaddr_in cli_addr;
	listen(serverSocket, 5);
	int cliLen = sizeof(cli_addr);

	// Wait for connections
	// add commands to queue
	// dispatcher takes command from queue and assigns it to new worker thread
	// worker thread executes command
	// write results to log
	// todo: put this in own thread
	while (1)
	{
		printf("Waiting for connections...\n");

		// Accept actual connection from the client
		int clientSocket = accept(serverSocket, (struct sockaddr *) &cli_addr,&cliLen);
		if (clientSocket < 0)
		{
			perror("Error on accept\n");
			exit(1);
		}
		printf("Client connected: %d \n", cli_addr.sin_addr.s_addr);

		char PUFFER[MAX_COMMAND_LENGTH];	// Puffer
		ssize_t numBytesRcvd = recv(clientSocket, PUFFER, MAX_COMMAND_LENGTH, MSG_WAITALL);
		
		close(clientSocket);
		//printf("Client disconnected.\n");
		
		PUFFER[numBytesRcvd] = '\0';
		enqueue(PUFFER);
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
	if (bind(serverSocket, (struct sockaddr *) &serv_addr, sizeof(serv_addr))
			< 0)
	{
		perror("Error on binding\n");
		exit(1);
	}

	return serverSocket;
}

// Takes command from queue and assigns to new worker thread
void *dispatcher(void *arg)
{
	printf("Wartelistelänge: %d \n", command_queue->counter);

	while (true)
	{
		if (!isEmpty())
		{
			if (worker_thread_counter < NR_MAX_WORKERS)
			{
				char *com = dequeue();
				//printf("Test dispatcher Command: %s\n", com);
				pthread_create(&(worker_threads[worker_thread_counter]), NULL, worker, com); // letzer NUll-Wert: Übergabeparameter
				worker_thread_counter++;
			}
		}
	}
}

void *worker(void *arg)
{
	char *command = (char*)arg;

	printf("Command for worker: %s\n", command);

	char header = command[0];
	command += 2;

	switch(header)
	{
		case '1':
			printf("Befehl: 1\n");
			executeCommand(command);
			break;
		case '2':
			break;
		case '3':
			break;
	}

	// Exit thread:
	
	pthread_exit(NULL);
}

void enqueue(char *command)
{
	if (command_queue->counter < NR_MAX_WORKERS)
	{
		Node* tmp = (Node*) malloc(sizeof(Node));

		//printf("Test enqueue: %s \n", command);
		
		tmp->value = command;

		if (command_queue->head == NULL)	// Erstes Element
			command_queue->head = tmp;
		else
			// jedes weitere
			command_queue->tail->next = tmp;

		tmp->prev = command_queue->tail;
		command_queue->tail = tmp;
		command_queue->counter++;
	}
}

char *dequeue()
{
	Node* tmp = command_queue->head;
	command_queue->head = command_queue->head->next;

	if (command_queue->head == NULL)
		command_queue->tail = NULL;
	else
		command_queue->head->prev = NULL;

	char *data = tmp->value;
	if (tmp != NULL)
	{
		command_queue->counter--;
		tmp->next = NULL;
		free(tmp);
	}
	
	return data;
}

bool isEmpty()
{
	if (command_queue->counter == 0)
		return true;
	else
		return false;
}

void writeToLog(char *text)
{
	FILE *f = fopen("server_log.txt", "a");
	if (f == NULL)
	{
	    printf("Error opening file!\n");
	    exit(1);
	}

	/* print some text */
	//const char *text = "Write this to the file";
	fprintf(f, "%s\n", text);

	fclose(f);
}

char *executeCommand(char *com)
{
	FILE *fp;
  	char path[1035];

  	writeToLog(com+": \n");
  	/* Open the command for reading. */
	fp = popen(com, "r");
	if (fp == NULL) 
	{
		writeToLog("Failed to run command \n");
    	printf("Failed to run command\n" );
    	exit(1);
  	}

	/* Read the output a line at a time - output it. */
  	while (fgets(path, sizeof(path)-1, fp) != NULL) 
  	{
    	//printf("%s", path);
    	writeToLog(path);
  	}
  	/* close */
  	pclose(fp);

  	return 0;
}