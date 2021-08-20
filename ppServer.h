//FILE ppServer.h

#ifndef HEADER_PPSERVER
#define HEADER_PPSERVER
#define SERVER_PORT 8080

#include <stdio.h>

/*************************************************************/

// structures

//node for a linked list conaining specific pokemon info
typedef struct Node{
	char* data;
	struct Node* next;
}PokeNodeType;

//multiple arguments to pass into a thread
struct args{
	FILE* fid;
	char* input;
	PokeNodeType* head;
	struct FileNamesType** names;
};

/*************************************************************/

// prototypes 

int fexists(char *fileName);
void* typeSearch(void *arg);
void createPokeNode(PokeNodeType** node, char* data);
void printPoke(PokeNodeType* node);
void sendList(PokeNodeType* node, int clientSocket);
void freePoke(PokeNodeType* node);

/*************************************************************/

#endif
