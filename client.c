/**********************************************************************
*
* FILENAME: client.c
*
* DESCRIPTION:
*       Represents the client of the multihreaded client/server
*       application. Expects commands from a user interprets them
*       and sends them to the server.
*
* AUTHOR: Dominik Rauh
*
**********************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <netdb.h>
#include <signal.h>
#include <fcntl.h>
#include <netinet/ip.h>
#include <pthread.h>

#define COMMAND_LENGTH 255
#define SERVER_NAME "127.0.0.1"
#define SERVER_PORT 1417

bool process_commmand(char *command);
void send_command(char *command);
void print_help();
struct sockaddr_in get_ipa(const char*, int);

/*
* Starts the client, by reading the user's input
* from stdin (keyboard)
*/
int main (int argc, char *argv[])
{
    setbuf(stdout, NULL);
    bool exit = false;
    char command[COMMAND_LENGTH];
    do
    {
        printf("Type in your command: ");
        fgets(command, sizeof(command), stdin);
        command[strlen(command) - 1] = '\0';

        printf("\n");
        exit = process_commmand(command);
        printf("\n");

    }while(!exit);

    return EXIT_SUCCESS;
}

/*
* Interprets the passed command and determines whether it
* should be send to the server
*
* command: The command to intepret
*
* returns: true if the client should be quit, because
*          because the q-Command has been put in by the user
*/
bool process_commmand(char *command)
{
    bool exit = false;

    if(*command == 'q')
    {
        exit = true;
        printf("Quitting\n");
    }
    else if(*command == 'h')
    {
        print_help();
    }
    else if((strlen(command) > 0) && ((*command == '1') || (*command == '2') || (*command == '3')))
    {
        send_command(command);
    }
    else
    {
        printf("Error invalid command!\n");
    }

    return exit;
}

/*
* Sends the passed command to the server through a socket
* defined by the name and port macros at the beginning
*
* command: The command string to send to the server
*/
void send_command(char *command)
{
    bool error = false;
    struct protoent* tcp;

    tcp = getprotobyname("tcp");

    int sfd = socket(PF_INET, SOCK_STREAM, tcp->p_proto);

    if(sfd == -1)
    {
        printf("Error creating a tcp socket.\n");
        error = true;
    }

    struct sockaddr_in isa = get_ipa(SERVER_NAME, SERVER_PORT);

    if(connect(sfd, (struct sockaddr*)&isa, sizeof isa) == -1)
    {
        printf("Error connecting to server.\n");
        error = true;
    }

    if(send(sfd, (void*) command, strlen(command), MSG_NOSIGNAL) == -1)
    {
        printf("Error sending message to server.\n");
        error = true;
    }

    if(error)
    {
        printf("\n");
        printf("Error: command \"%s\" could not be sent to server\n", command);
    }
    else
    {
        printf("Command \"%s\" successfully sent to server\n", command);
    }

    shutdown(sfd, SHUT_RDWR);
}

/*
* Prints a description of all available commands
*/
void print_help()
{
    printf("q [Quit client]\n");
    printf("h [Print help]\n");
    printf("1,<shell command> [Execute the shell command on the server]\n");
    printf("2,<n> [Calculate the n-th fibonacci number]\n");
    printf("3,shutdown server [Shuts the server down]\n");
}

/*
* Creates a structure which contains the server's
* hostname (respectively ip address) and port.
* The structure can then be used by a socket
* implementation to connect to the specified
* remote socket
*
* hostname: The specified hostname
* port: The specified port
*
* returns: A structure containing the passed params
*/
struct sockaddr_in get_ipa(const char *hostname, int port)
{
    struct sockaddr_in ipa;
    ipa.sin_family = AF_INET;
    ipa.sin_port = htons(port);

    struct hostent* server = gethostbyname(hostname);
    if(!server)
    {
        printf("Error resolving server\n");
	return ipa;
    }

    char* addr = server->h_addr_list[0];
    memcpy(&ipa.sin_addr.s_addr, addr, sizeof addr);

    return ipa;
}
