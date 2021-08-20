/*********************************************************************/
/* File is ppServer.c
 *
 * Purpose: Creates a server to allow a client to connect and read 
 * Pokemon data from a file, and save all pokemon of a specific type 
 * to a separate file
 *
 * Ethan Doherty - 101200654 - 2021-08-07
 *
 */
/*************************************************************/

// INCLUDES 
//
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "ppServer.h"

/*************************************************************/
/* purpose: creates a server and communicates with the client
 *
 * local variables:
 * serverSocket, clientSocket, serverAddress, clientAddr, status,
 * addrSize, bytesRcv - variables for construction of the server
 
 * char buffer[] - a message recieved
 * char* response - a default response to remain synchronized with the client
 * struct args* searchArgs - arguments to pass to a function
 * PokeNodeType* pokeInfo - a linked list containing pokemon data
 * char* fileName - the name of a file
 * FILE *fid - a file id
 * int rc - a return code
 *
 * char* choice - a user input
 * struct FileNamesType* createdFiles - a linked list containing file names
 * int numFiles - the number of files created 
 */

int main() {
	int serverSocket, clientSocket;
	struct sockaddr_in serverAddress, clientAddr;
	int status, addrSize, bytesRcv;
	
	char buffer[30];
	char *response = "OK";
	struct args* searchArgs = NULL;
	PokeNodeType* pokeInfo = NULL;
	char* fileName = NULL;
	FILE *fid;
	int rc = 0;
	
	//get file name from user
	do{
		printf("Input the file name or 'q' to quit.\n");
		scanf("%ms",&fileName);
		
		if(strcmp(fileName, "q") == 0){
			free(fileName);
			exit(-1);
		}
		
		// check if file exists
		rc = fexists(fileName);

		// print message depending whether the file exists
		if(!rc){
			free(fileName);
		}
		
	}while(!rc);

	// Create the server socket 
	serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (serverSocket < 0) {
		printf("*** SERVER ERROR: Could not open socket.\n");
		free(fileName);
		exit(-1);
	}
	
	// Setup the server address
	memset(&serverAddress, 0, sizeof(serverAddress)); // zeros the struct
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_addr.s_addr = inet_addr("127.0.0.1");
	serverAddress.sin_port = htons((unsigned short) SERVER_PORT);
	
	// Bind the server socket 
	status = bind(serverSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress));
	if (status < 0) {
		printf("*** SERVER ERROR: Could not bind socket.\n");
		free(fileName);
		exit(-1);
	}
	
	// Set up the line-up to handle up to 5 clients in line
	status = listen(serverSocket, 5);
	if (status < 0) {
		printf("*** SERVER ERROR: Could not listen on socket.\n");
		free(fileName);
		exit(-1);
	}
	
	// Wait for clients now
	while (1) {
		addrSize = sizeof(clientAddr); 
		clientSocket = accept(serverSocket,(struct sockaddr *)&clientAddr,&addrSize);
		if (clientSocket < 0) {
			printf("*** SERVER ERROR: Could accept incoming client connection.\n");
			free(fileName);
			exit(-1);
		}
		printf("SERVER: Received client connection.\n");
		// Go into infinite loop to talk to client
		
		searchArgs = malloc(sizeof(struct args));
		while (1) {
			// Get the message from the client 
			bytesRcv = recv(clientSocket, buffer, sizeof(buffer), 0);
			buffer[bytesRcv] = 0; // put a 0 at the end so we can display the string
			printf("SERVER: Received client request: %s\n", buffer);
			
			if(*buffer == '1'){
				fid = fopen(fileName, "r");
				char* type = NULL;
				PokeNodeType* searchRet;
		
				//get the type
				printf("Select a type (eg: Fire): \n");
				scanf("%ms", &type);
				
				//set arguments
				searchArgs->fid = fid;
				searchArgs->input = type;
				searchArgs->head = pokeInfo;
				searchArgs->names = NULL;
				
				searchRet = (PokeNodeType *) typeSearch(searchArgs);
				sendList(searchRet, clientSocket);
				freePoke(searchRet);
				
				if(fclose(fid) == EOF){
					printf("Error closing the file\n");
					break;
				}
			}
			// Respond with an "OK" message
			send(clientSocket, response, strlen(response), 0);
			
			if (strcmp(buffer,"3") == 0 || strcmp(buffer,"4") == 0){
				break;
			}
			
		}
		free(searchArgs);
		printf("SERVER: Closing client connection.\n");
		close(clientSocket); // Close this client's socket
		// If the client said to stop, then I'll stop myself
		if (strcmp(buffer,"4") == 0)
			break;
	}
	free(fileName);
	
	// Don't forget to close the sockets!
	close(serverSocket);
	printf("SERVER: Shutting down.\n");
}

/*********************************************************************/
/* Purpose: return whether the given file exists in the current directory.
 * 
 * input:
 * char* fileName - name of file
 *
 * return:
 * 0 if the file does not exist
 * 1 if the file exists
 *
 * local variables:
 * FILE *fp - a file id
 * int rc - a return code
 *
 */ 
 
int fexists(char *fileName)
{

	FILE *fp = NULL;
	int rc = 0;

	// open the file
	fp = fopen(fileName, "r");
	if(!fp){
		printf("Pokemon file is not found. Please enter the name of the file again.\n");
		return(rc);
	}
	if(fclose(fp) == EOF){
		printf("Error closing the file\n");
		return(rc);
	}
	else{
		rc = 1;
	}

	return(rc);
}
/*********************************************************************/
/* Purpose: searches for pokemon of a requested type and saves them to
 *		a linked list
 * input:
 * void* arg
 *	char* type - the requested type
 *	FILE* fid - the file id of the pokemon info
 *
 * return:
 * head - a reference to the head node
 *
 * local variables:
 * char* type - the type input from user
 * char data[100] - a line from pokemon.csv
 * char* tempStr - heap duplicate of data
 * char* token - return from strtok()
 * FILE* fid - a file id
 * PokeNodeType* oldHead, tail, head, newNode - pointers to nodes in a linked list
 * int saved - flag for freeing memory when a pokemons data doesn't match requested type
 *
 */
 
void* typeSearch(void *arg){
	char* type = ((struct args*) arg)->input;
	char data[100];
	char* tempStr = malloc(sizeof(data)+ sizeof(char));
	char* token;
	FILE* fid = ((struct args*) arg)->fid;
	
	PokeNodeType* oldHead = ((struct args*)arg)->head;
	PokeNodeType* tail;
	PokeNodeType* head = oldHead;
	PokeNodeType* newNode;
	
	if(head == NULL){
		//first line in file becomes head of linked list
		fscanf(fid, "%[^\n]%*c", data); //%[^\n]for spaces
		strcpy(tempStr, data);
		createPokeNode(&newNode, tempStr);
		oldHead = newNode;
		head = newNode;
		tail = newNode;
		tempStr = (char*)malloc(sizeof(data)+ sizeof(char));
	}
	else{
		//start head at the end of a previously allocated linked list
		while(head->next != NULL){
			head = head->next;
		}
		tail = head;
	}
	
	
	//find pokemon of inputed type
	int saved;
	while(fscanf(fid, "%[^\n]%*c", data) == 1){
		saved = 0;
		strcpy(tempStr, data);
		token = strtok (data,",");
		
		// i < 3 since type 1 is the third element
		for(int i = 0; i < 3; i++){
			//skip number and name
			if(i < 2){
				token = strtok (NULL, ",");
				continue;
			}
		
			//save correct type to linked list
			if(strcmp (token, type) == 0){
				createPokeNode(&newNode, tempStr);
				tail->next = newNode;
				tail = tail->next;
				saved = 1;
				break;
			}
			token = strtok (NULL, ",");
		}
		newNode = NULL;
		
		if(!saved){
			free(tempStr);
		}
		tempStr = (char*)malloc(sizeof(data)+ sizeof(char));
	}
	free(tempStr); //the last malloc happens on EOF
	free(((struct args*) arg)->input);

	return (void *) oldHead;
	
}

/*********************************************************************/
/* Purpose: creates a node for a PokeNodeType linked list
 *
 * input: 
 * PokeNodeType** node - reference to a node pointer to store data in
 * char* data - reference to pokemon data to store in linked list
 *
 */
 
void createPokeNode(PokeNodeType** node, char* data){
	*node = (PokeNodeType*) malloc(sizeof(PokeNodeType));
	if(node == NULL){
		printf("Memory allocation error\n");
		exit(0);
	}
	(*node)->data = data;
	(*node)->next = NULL;
}



/*********************************************************************/
/* Purpose: frees memory used by PokeNodeType linked list
 *		only used when not writing to a file
 *
 * input: 
 * PokeNodeType* node - reference to a node pointer to store data in
 *
 * local variables:
 * PokeNodeType* currNode, temp - pointers to nodes in a linked list
 */
 
  //this function is not used, but is a useful tool for debugging
void printPoke(PokeNodeType* node){
	PokeNodeType* currNode = node;
 	PokeNodeType* temp;

 	while(currNode != NULL){
		temp = currNode->next;
		printf("%s\n", currNode->data);
		currNode = temp;	
	}
 }
 /*********************************************************************/
 /* Purpose: sends a linked list of PokeNodes over to the client
  *
  * input:
  * PokeNodeType* node - a head node to the linked list to be sent
  * int clientSocket - the id of the client socket to send to
  *
  * local variables:
  * int listSize - the size of the linked list
  * PokeNodeType* currNode - a pointer to a node in the linked list while iterating
  * char charList - a char version of int listSize
  * char buffer - a recieved message to stay in sync with the client
  */
 void sendList(PokeNodeType* node, int clientSocket){
 	int listSize = 0;
 	PokeNodeType* currNode = node;
 	char charList[4]; //127 size linked list on water types
 	char buffer[30];
 	
 	while(currNode != NULL){
 		listSize++;
 		currNode = currNode->next;
 	}
 	
 	sprintf(charList, "%i", listSize);
 	send(clientSocket, charList, strlen(charList), 0);
 	recv(clientSocket, buffer, sizeof(buffer), 0);
 	
 	currNode = node;
	while(currNode != NULL){
		send(clientSocket, currNode->data, strlen(currNode->data), 0);
		recv(clientSocket, buffer, sizeof(buffer), 0);
		
		currNode = currNode->next;
	}
 }
 /*********************************************************************/
/* Purpose: frees memory used by PokeNodeType linked list
 *		only used when not writing to a file
 *
 * input: 
 * PokeNodeType* node - reference to a node pointer to store data in
 *
 * local variables:
 * PokeNodeType* currNode, temp - pointers to nodes in a linked list
 */
void freePoke(PokeNodeType* node){
	PokeNodeType* currNode = node;
 	PokeNodeType* temp;

 	while(currNode != NULL){
		temp = currNode->next;
		free(currNode->data);
		free(currNode);
		currNode = temp;	
	}
 }
