/*********************************************************************/
/* File is pqClient.c
 *
 * Purpose: Creates a client to connect to a server and read 
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
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "pqClient.h"

/*************************************************************/
/* purpose: creates a client and communicates with the server
 *
 * local variables:
 * clientSocket, serverAddress, status - variables for 
 * communication with the server
 
 * char choice - a user input for a menu
 * PokeNodeType* pokeInfo - a linked list containing pokemon data
 * struct args* searchArgs - arguments to pass to a function
 * struct FileNamesType* createdFiles - a linked list containing file names
 * char buffer[] - a message recieved
 
 * char* fileName - the name of a file
 * FILE *fid - a file id
 * int rc - a return code
 *
 * char* choice - a user input
 * int numFiles - the number of files created 
 */
 
int main() {
	int clientSocket;
	struct sockaddr_in serverAddress;
	int status, bytesRcv;
	
	char* choice; // stores user input from keyboard
	PokeNodeType* pokeInfo = NULL;
	struct args* searchArgs;
	struct FileNamesType* createdFiles = NULL;
	char buffer[101]; // stores user input from keyboard
	
	// Create the client socket
	clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (clientSocket < 0) {
		printf("*** CLIENT ERROR: Could not open socket.\n");
		exit(-1);
	}
	
	// Setup address
	memset(&serverAddress, 0, sizeof(serverAddress));
	serverAddress.sin_family = AF_INET; serverAddress.sin_addr.s_addr = inet_addr(SERVER_IP);
	serverAddress.sin_port = htons((unsigned short) SERVER_PORT);
	
	// Connect to server 
	status = connect(clientSocket, (struct sockaddr *) &serverAddress,
	sizeof(serverAddress));
	if (status < 0) {
		printf("Unable to establish connection to the PPS!\n");
		exit(-1);
	}
	
	pthread_t t1; //thread
	void* status_p1 = NULL; //thread return value
	int t1_running = 0; //thread status
	
	 searchArgs = malloc(sizeof(struct args));
	
	// Go into loop to commuincate with server now
	while (1) {
		// Get a command from the user
		choice = (char *)dispMsg();
		
		// Send command string to server
		strcpy(buffer, choice);
		printf("CLIENT: Sending \"%s\" to server.\n", buffer);
		send(clientSocket, buffer, strlen(buffer), 0);
		
		if(strcmp(choice, "1") == 0){
			int listSize;
			PokeNodeType* newNode;
			PokeNodeType* tail;
			char* returnMsg = "ok";
			char* bufferCpy;
			
			//close open threads
			if(t1_running){
				if(pthread_join(t1, &status_p1)){
					printf("ERROR joining t1");
					break;
				}
				free(status_p1);
				pokeInfo = NULL;
				t1_running = 0;
			}
			
			bytesRcv = recv(clientSocket, buffer, sizeof(buffer), 0);
			send(clientSocket, returnMsg, strlen(returnMsg), 0);
			buffer[bytesRcv] = 0; // put a 0 at the end so we can display the string
			listSize = atoi(buffer);
			
			//reconstruct the pokemon linked list from the server
			for(int i = 0; i < listSize; i++){
				bytesRcv = recv(clientSocket, buffer, sizeof(buffer), 0);
				send(clientSocket, returnMsg, strlen(returnMsg), 0);
				buffer[bytesRcv] = 0; // put a 0 at the end so we can display the string
				bufferCpy = malloc(sizeof(buffer));
				strcpy(bufferCpy, buffer);
				createPokeNode(&newNode, bufferCpy);
				if(pokeInfo == NULL){
					pokeInfo = newNode;
					tail = newNode;
				}
				else{
					tail->next = newNode;
					tail = tail->next;
				}
				
			}
		}
		else if(*choice == '2'){
			char* writeTo;
			
			//close open threads
			if(t1_running){
				if(pthread_join(t1, &status_p1)){
					printf("ERROR joining t1");
					break;
				}
				free(status_p1);
				pokeInfo = NULL;
				t1_running = 0;
			}
			
			//get the filename
			printf("Enter a file name to write to ('q' to quit): \n");
			scanf("%ms", &writeTo);
		
			if(pokeInfo != NULL){
				//set arguments
				searchArgs->fid = NULL;
				searchArgs->input = writeTo;
				searchArgs->head = pokeInfo;
				searchArgs->names = &createdFiles;
				
				//create a file containing pokemon data in the background
				if(pthread_create(&t1, NULL, createFile, searchArgs)){
					printf("ERROR creating t3");
					break;
				}
				t1_running = 1;
				
				pokeInfo = NULL;
			}
			else{
				free(writeTo);
				printf("You must perform a type search first\n");
			}
		}
		
		// Get response from server, should be "OK" 
		bytesRcv = recv(clientSocket, buffer, 80, 0);
		buffer[bytesRcv] = 0; // put a 0 at the end so we can display the string

		if (strcmp(choice,"3") == 0 || strcmp(choice,"4") == 0)
			break;
		
		free(choice);
	}
	
	if(t1_running){
		if(pthread_join(t1, &status_p1)){
			printf("ERROR joining t1");
			exit(-1);
		}
		free(status_p1);
		pokeInfo = NULL;
		t1_running = 0;
	}
	
	close(clientSocket); // Don't forget to close the socket !
	printf("CLIENT: Shutting down.\n");
	
	int numFiles = 0;
	struct FileNamesType* temp;
	
	printf("\n///////////////////////////\nList of all created files: \n");
	while(createdFiles != NULL){
		numFiles++;
		printf("%s\n", createdFiles->data);
		temp = createdFiles->next;
		free(createdFiles->data);
		free(createdFiles);
		createdFiles = temp;
	}
	
	printf("\nThe number of files created was: %d.\n", numFiles);
	free(choice);
	free(searchArgs);
	freePoke(pokeInfo);
}

/*********************************************************************/
/* Purpose: gets user input for a choice and returns it as a void pointer
 *		this function allows for a pthread to access this info
 *
 * return:
 * choice - the user input
 *
 * local variables:
 * char* choice - a user input
 *
 */
void* dispMsg(){
	char* choice = malloc(2* sizeof(char));
	
	printf("\nSelect an option: \n(1) Type Search \n(2) Save Results \n(3) Exit the program \n");
	scanf(" %c", choice);
	choice[1] = 0;
	return (void *) choice;
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
/* Purpose: creates a file to save a PokeNodeType linked list to
 *
 * input:
 * PokeNodeType* pokeInfo - reference to a head node of pokemon info
 * FileNamesType** createdFiles - a reference to a head node of filenames
 *
 * return:
 * ret - pointer to garbage value
 *
 * local variables:
 * PokeNodeType* currNode, temp - pointers to nodes in a linked list
 * char* fileName - a file name
 * FILE* fid - a file id
 * struct FileNamesType* current, newNode - pointers to nodes in a linked list
 * int ret - return value
 *
 */
 
void* createFile(void* arg){
	PokeNodeType* currNode = ((struct args*)arg)->head;
	char* fileName = ((struct args*)arg)->input;
	FILE* fid = NULL;
	struct FileNamesType* current = *((struct args*)arg)->names;
	struct FileNamesType* newNode;
	char* ret = malloc(sizeof(char));
	
	//get filename
	do{	
		if(strcmp(fileName, "q") == 0){
			return (void*)ret;
		}
		
		fid = fopen(fileName, "w");
		if(!fid){
			printf("Unable to create the new file. Please enter the name of the file again.\n");
		}
	}while(!fid);
	
	//write to file and free memory
	PokeNodeType* temp;
	while(currNode != NULL){
		fprintf(fid, "%s\n", currNode->data);
		temp = currNode->next;
		free(currNode->data);
		free(currNode);
		currNode = temp;
		
	}
	
	//store file name
	createFileNode(&newNode, fileName);
	if(*((struct args*)arg)->names == NULL){
		*((struct args*)arg)->names = newNode;
	}
	else{
		while(current->next != NULL){
			current = current->next;
		}
		current->next = newNode;
	}
	
	
	//close file
	if(fclose(fid) == EOF){
		printf("Error closing the file\n");
		return (void*)ret;
	}

	return (void*)ret;
}
/*********************************************************************/
/* Purpose: creates a node for a FileNamesType linked list
 *
 * input: 
 * FileNamesType** node - reference to a node pointer to store data in
 * char* data - reference to a file name to store in linked list
 *
 */
 
void createFileNode(struct FileNamesType** node, char* data){
	*node = (struct FileNamesType*) malloc(sizeof(struct FileNamesType));
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
