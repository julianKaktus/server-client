#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <semaphore.h>

#define MAX_COMMAND_LENGTH 255
#define SERVER_PORT 1417
#define NR_MAX_WORKERS 2

typedef int bool;
#define true 1
#define false 0

// Node for LinkedList
typedef struct Node
{
	struct Node *next;
	struct Node *prev;
	char *value;
} Node;

//LinkedList for the command queue
typedef struct
{
	struct Node* head;
	struct Node* tail;
	int counter;
	pthread_mutex_t mutex;	// needed to add/remove data from the queue
	pthread_cond_t can_produce;	// signaled when items are removed
	pthread_cond_t can_consume;	// signaled when items are added
} List;

List command_queue; 				// CommandQueue

pthread_t worker_threads[NR_MAX_WORKERS];	// Array of all workers
int worker_thread_counter = 0;			// Current workercount
pthread_mutex_t worker_thread_counter_mutex;	// Mutex to lock access on worker_thread_counter
sem_t create_worker_sema;			// Semaphore to prohibit to much workers

pthread_mutex_t lock;				// Mutex for server log

// List functions:
void enqueue(char *command);
char *dequeue();

// Dispatcher thread (consumer):
void *dispatcher(void *);

// Worker thread:
void *worker(void *);

// Helper functions:
void writeToLog(char *text);
char *executeCommand(char *com);
int getNFibnumber(char *com);

int main(int argc, char *argv[])
{
	// Init queue:
	command_queue.head = command_queue.tail = NULL;
	command_queue.counter = 0;

	pthread_mutex_init(&command_queue.mutex, NULL);
	pthread_cond_init(&command_queue.can_produce, NULL);
	pthread_cond_init(&command_queue.can_consume, NULL);
	
	// Init worker-counter-access mutex and semaphore
	pthread_mutex_init(&worker_thread_counter_mutex, NULL);
	sem_init(&create_worker_sema, 0, NR_MAX_WORKERS);
	
	// Start dispatcher thread:
	pthread_t dispatcher_thread;
	pthread_create(&dispatcher_thread, NULL, dispatcher, NULL);

	// Init server
	int serverSocket = initServer();
	
	// Init Mutex for server log:
	if (pthread_mutex_init(&lock, NULL) != 0 )
	{
		perror("Mutex init failed\n");
		exit(1);
	}

	struct sockaddr_in cli_addr;
	listen(serverSocket, 5);
	int cliLen = sizeof(cli_addr);

	printf("Server started. Waiting for connections...\n");

	// Wait for connections
	// add commands to queue
	// dispatcher takes command from queue and assigns it to new worker thread
	// worker thread executes command
	// write results to log
	while (1)
	{
		// Accept actual connection from the client
		int clientSocket = accept(serverSocket, (struct sockaddr *) &cli_addr,&cliLen);
		if (clientSocket < 0)
		{
			perror("Error on accept\n");
			exit(1);
		}
		//printf("Client connected: %d \n", cli_addr.sin_addr.s_addr);

		// Receive message from client
		char PUFFER[MAX_COMMAND_LENGTH];	// Puffer
		ssize_t numBytesRcvd = recv(clientSocket, PUFFER, MAX_COMMAND_LENGTH, MSG_WAITALL);
		PUFFER[numBytesRcvd] = '\0';

		close(clientSocket);
		
		// Server Shutdown command
		if (PUFFER[0] == '3')
		{
			printf("Shutdown server!\n");
			pthread_mutex_destroy(&lock);
			exit(1);
		}

		// Producer
		pthread_mutex_lock(&command_queue.mutex);

		while(command_queue.counter == NR_MAX_WORKERS)	// full
		{
			// wait until some elements are consumed
			char msg[256] = "Buffer is full. Wait until buffer has empty places\n";
			send(clientSocket, msg, 256, 0);
			printf(msg);
			pthread_cond_wait(&command_queue.can_produce, &command_queue.mutex);
		}
		char msg[256] = "Command enqueued and buffer not full.\n";
		send(clientSocket, msg, 256, 0);
		printf(msg);

		// Produce
		enqueue(PUFFER);

	// signal the fact that new items may be consumed
        pthread_cond_signal(&command_queue.can_consume);
        pthread_mutex_unlock(&command_queue.mutex);

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
	//printf("Socket opened.\n");

	// Initialize socket structure
	struct sockaddr_in serv_addr;

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(SERVER_PORT);

	// Bind host adress
	if (bind(serverSocket, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
	{
		perror("Error on binding\n");
		exit(1);
	}

	return serverSocket;
}

// Puts a command into the command queue
void enqueue(char *command)
{
	Node* tmp = (Node*) malloc(sizeof(Node));
	
	tmp->value = command;

	if (command_queue.head == NULL)		// First Element
		command_queue.head = tmp;
	else
		command_queue.tail->next = tmp;	// every next

	tmp->prev = command_queue.tail;
	command_queue.tail = tmp;
	command_queue.counter++;				// Counts the number of Elements in the queue
}

// Take command from command queue (FIFO)
// Delete memory
char *dequeue()
{
	Node* tmp = command_queue.head;
	command_queue.head = command_queue.head->next;

	if (command_queue.head == NULL)
		command_queue.tail = NULL;
	else
		command_queue.head->prev = NULL;

	char *data = tmp->value;
	if (tmp != NULL)
	{
		command_queue.counter--;
		tmp->next = NULL;
		free(tmp);
	}
	
	return data;
}

// Takes command from queue and starts new worker thread
void *dispatcher(void *arg)
{
	while(true) 
	{
		pthread_mutex_lock(&command_queue.mutex);

		while (command_queue.counter == 0) 	// Queue empty
		{
			// wait for new items to be appended to the buffer
			pthread_cond_wait(&command_queue.can_consume, &command_queue.mutex);
		}

		
		sem_wait(&create_worker_sema);
		pthread_mutex_lock(&worker_thread_counter_mutex);		
		
		
		printf("Dispatcher: workerCounter=%d\n", worker_thread_counter);
		// Consume
		
		char *com = dequeue();
		printf("Dispatcher: Consumed\n");
		
		if(pthread_create(&(worker_threads[worker_thread_counter]), NULL, worker, com) != 0) // letzer NUll-Wert: Übergabeparameter
		{
			perror("Dispatcher: Konnte Thread nicht erzeugen\n");
			exit(1);
		}
		worker_thread_counter++;
		pthread_mutex_unlock(&worker_thread_counter_mutex);
		// signal the fact that new items may be produced
        pthread_cond_signal(&command_queue.can_produce);
        pthread_mutex_unlock(&command_queue.mutex);
	}

	return NULL;
}

// The worker thread gets a command as argument and decides which task 
// to execute 
void *worker(void *arg)
{
	printf(">Worker thread 0x%lX started\n", pthread_self());
	char *command = (char*)arg;

	char header = command[0];
	command += 2;

	// Check number of message
	switch(header)
	{
		case '1':			// Execute shell command
			printf("-- Starte Task 1 mit Kommando: %s \n", command);
			executeCommand(command);
			break;
		case '2':			// Get the n-th number of Fibonacci series
			printf("-- Starte Task 2 mit Parameter %s \n", command);
			int fib = getNFibnumber(command);
			char buf[10];	// Max nr of digits for a fibonacci number: 10
			sprintf(buf, "%d", fib);	// Convert to string for the output
			writeToLog(buf);
			break;
		default:
			printf("Command not defined!\n");
			break;
	}
	// Lock mutex to access worker_thread_counter
	pthread_mutex_lock(&worker_thread_counter_mutex);
		printf("-Worker thread 0x%lX ended\n", pthread_self());
		worker_thread_counter--;
		sem_post(&create_worker_sema); // Signal to semaphore that this thread is finished
	pthread_mutex_unlock(&worker_thread_counter_mutex);	
	// Exit thread:
	pthread_exit(NULL);
}


// Writes a specified text to a file called 'server_log.txt'
void writeToLog(char *text)
{
	pthread_mutex_lock(&lock);

	FILE *f = fopen("server_log.txt", "a");
	if (f == NULL)
	{
	    printf("Error opening file!\n");
	    exit(1);
	}

	/* print text */
	fprintf(f, "%s\n", text);

	fclose(f);

	pthread_mutex_unlock(&lock);
}

// Executes a shell command and writes output into global server log
char *executeCommand(char *com)
{
	FILE *fp;
  	char path[1035];

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

// Calculates the n-th fibonacci number
int getNFibnumber(char *com)
{
	// Convert string to int
	int n = atoi(com);
	
	int a = 0;
	int b = 1;
	int c;

	if ( n == 0)
	{
		return a;
	}

	for (int i = 2; i <= n; i++)
	{
		c = a + b;
		a = b;
		b = c;
	}

	return b;
}
