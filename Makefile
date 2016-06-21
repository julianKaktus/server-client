# compiler options -- C99 with warnings
OPT_GCC = -std=c99 -Wall -Wextra -g
OPT = -D_XOPEN_SOURCE=700
LIB = -lpthread -lrt

all: server client

client: client.c
	gcc $(OPT_GCC) $(OPT) -o client client.c $(LIB)

server: server.c
	gcc $(OPT_GCC) $(OPT) -o server server.c $(LIB)

run: server client
	./client
	./server	

clean:
	rm -f server
	rm -f client
	rm -f server_log.txt
