//FILE pqClient.h

#ifndef HEADER_PQCLIENT
#define HEADER_PQCLIENT
#define SERVER_PORT 8080
#define SERVER_IP "127.0.0.1"

#include <stdio.h>

/*************************************************************/

// structures

//node for a linked list conaining specific pokemon info
typedef struct Node{
	char* data;
	struct Node* next;
}PokeNodeType;

//node for a linked list containing all created filenames
struct FileNamesType{
	char* data;
	struct FileNamesType* next;
};

//multiple arguments to pass into a thread
struct args{
	FILE* fid;
	char* input;
	PokeNodeType* head;
	struct FileNamesType** names;
};
/*************************************************************/

// prototypes 

void* dispMsg();
void printPoke(PokeNodeType* node);
void createPokeNode(PokeNodeType** node, char* data);
void* createFile(void* arg);
void createFileNode(struct FileNamesType** node, char* data);
void freePoke(PokeNodeType* node);

/*************************************************************/

#endif
