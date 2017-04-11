/* CSci4061 Assignment 3
 * name: Zachary Vollen, Yadu Kiran
 * id: voll0139, kiran013*/
 
//Date: 11/18/2015

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/stat.h>
#include <semaphore.h>
#include "makeargv.h"

//#define DEBUG

//database entry structure
typedef struct DataBaseEntry {
	char *city;
	char *word1;
	char *word2;
	char *word3;
} DBE;

//linked list node structure
typedef struct Node {
	char* path;
	struct Node *next;
} node_t;

//initialize linked list
node_t *head;
node_t *tail;

//number of threads
int numThreads;

//keep track of number of nodes in queue
int inqueue = 0;

int iter = 0;
int a[50];

//number of clients
int numClients;
int numClientsLeft;

//Declare a structure that holds the lines from the Database
DBE DataBase[100];
int dbSize = 0;

//Declare a structure that holds the Client Names
char *clientPath[100];
int clientIdx = 0;

//mutex locks and semaphores
sem_t s1, s2, s3;
pthread_mutex_t mutex=PTHREAD_MUTEX_INITIALIZER;

//function declarations
void serviceClient(char *path);
void processThread();
void getDataBase();
void populate();
void addNode();

void printNodes();

void *pool(void *id);
int readInFile(char* path);
node_t* popNode();

int main(int argc, char *argv[])
{
	//Check if the user has entered the correct number of Arguments
	if(argc != 3)
	{
		fprintf(stderr,"Error: incorrect number of arguments\n Usage: ./twitterTrend (input_file_path) (num_threads)\n");
		exit(1);
	}

	if((numThreads = atoi(argv[2])) < 1)
	{
		fprintf(stderr,"insufficient number of threads or num_threads argument wasn't a valid integer.\n");
		exit(1);
	}

	//Read the Data Base File and import it to memory
	getDataBase();

	//Read the input file and extract the Client Names/Paths
	numClients = readInFile(argv[1]);
	numClientsLeft = numClients;
	
	//Declare threads.
	pthread_t threads[numThreads];
	
	//Initialize semaphores.
	sem_init(&s1, 0, 0);
	sem_init(&s2, 0, 0);
	sem_init(&s3, 0, numClients);
	
	
	#ifdef DEBUG
	printf("Number of Clients: %d\n", numClients);
	printf("Clients left: %d\n", numClientsLeft);
	printf("Number of threads: %d\n", numThreads);
	printf("Number of DB entries: %d\n\n", dbSize);
	#endif

	//Create the threads.
	int i;
	for(i = 0; i < numThreads; i++)
	{
		a[i]=i+1;
		pthread_create(&threads[i], NULL, pool, &(a[i]));
	}
	
	//Handles filling the queue.
	populate();
	
	//Wait for threads to finish executing.
	for(i = 0; i < numThreads; i++)
	{
		pthread_join(threads[i], NULL);
	}

	#ifdef DEBUG
	fprintf(stdout, "Finished Execution.\n");
	#endif
	
	return 0;
}

//Gets twitter Database and stores it in a DatabaseEntry (DBE) array.
void getDataBase()
{
	//Open the twitter Database.
	FILE *db;
	db = fopen("TwitterDB.txt","r");
	if(db == NULL)
	{
		perror("Error");
		exit(1);
	}
	
	//Extract one line at a time from the Database.
	char line[256];
	char *delimeter= ",";
	char **tokhold;
	int errchk = 0;
	int i = 0;
	while(fgets(line, 256, db) != NULL)
	{
		//Remove the new line character. 
		size_t ln = strlen(line) - 1;
		if (line[ln] == '\n')
		{
			line[ln] = '\0';
		}
		
		//Use make argv function to tokenize each word in a line.
		errchk = makeargv(line, delimeter, &tokhold);
		if(errchk == -1)
		{
			fprintf(stderr,"Error: Unable to Parse the Line");
			exit(1);
		}
		
		//Store the Tokens in the Database.
		DataBase[i].city=(char*)malloc(strlen(tokhold[0]));
		strcpy(DataBase[i].city, tokhold[0]);
		
		DataBase[i].word1=(char*)malloc(strlen(tokhold[1]));
		strcpy(DataBase[i].word1, tokhold[1]);
		
		DataBase[i].word2=(char*)malloc(strlen(tokhold[2]));
		strcat(DataBase[i].word2, tokhold[2]);
		
		DataBase[i].word3=(char*)malloc(strlen(tokhold[3]));
		strcat(DataBase[i].word3, tokhold[3]);
		++i;
		++dbSize;
	}
	
	//Close the file.
	if(fclose(db)!=0)
	{
		perror("Error");
		exit(1);
	}
}


//handles filling the shared queue.
void populate()
{
	if(numClientsLeft == 0)
	{
		return;
	}	
	//sem_wait(&s2);
	//fill queue
	while(1)
	{
		pthread_mutex_lock(&mutex);
		if(numClientsLeft == 0)
		{
			pthread_mutex_unlock(&mutex);
			break;
		}

		if(inqueue == numThreads)
		{
			pthread_mutex_unlock(&mutex);
			break;
		}
		addNode();
		--numClientsLeft;
		++inqueue;
		pthread_mutex_unlock(&mutex);
	}
	
	#ifdef DEBUG
	printNodes();
	#endif
	//Pool only enters this part of the code when inqueue==numThreads
	if(iter>0)
	{
		//Pool cannot add any more items in the queue till it posts the
		//semaphore and signals the pool function to pop items from the queue
		printf("Waiting to add clients to the full queue\n");
	}
	++iter;
	//signal threads to start processing
	int i;
	for(i = 0; i < inqueue; i++)
	{
		sem_post(&s1);
	}
		
	sem_wait(&s2);
	populate();
}

//reads the file passed in and stores all of the paths in an array.
int readInFile(char* path)
{
	//Open the .in file which contains the name of the clients
	FILE *InFile;
	InFile = fopen(path, "r");
	if(InFile == NULL)
	{
		perror("Error");
		exit(1);
	}
	
	//Read Each Line and store the name of the clients
	char line[128];
	int i = 0, check;
	
	while(fgets(line, 128, InFile) != NULL)
	{
		//Remove the New line present at the end 
		size_t ln = strlen(line) - 1;
		if (line[ln] == '\n')
		{
			line[ln] = '\0';
		}
		
		clientPath[i] = (char*)malloc(strlen(line));
		strcpy(clientPath[i], line);
		
		#ifdef DEBUG
		fprintf(stdout,"%s \n",clientPath[i]);
		#endif
		
		++i;
	}
	
	//Close the File
	if(fclose(InFile) != 0)
	{
		perror("Error");
		exit(1);
	}
	return i;
}

//main processing thread function, handles taking items from the queue and processing them.
void *pool(void *id)
{
	int ID = *(int*) id;
	
	if(sem_trywait(&s3) == -1)
	{
		pthread_exit(NULL);
	}
	sem_wait(&s1);
	int x;                      
	for(x=0;x<10000;++x){}
	pthread_mutex_lock(&mutex);
	node_t *node = popNode();
	--inqueue;
	fprintf(stderr, "Thread %d is handling client %s\n", ID, node->path);
	if(inqueue==0)
	{
		sem_post(&s2);
	}
	pthread_mutex_unlock(&mutex);
	
	
	serviceClient(node->path);
	fprintf(stderr, "Thread %d finished handling client %s\n", ID, node->path);
	
	free(node);
	pool(id);
}

/*
 * main thread process helper function that processes the client.txt file,
 * and writes the output file
 */
void serviceClient(char *path)
{	
	FILE *client;
	client = fopen(path, "r");
	if(client == NULL)
	{
		perror("Error 1");
		exit(1);
	}
	
	//get the city from client.txt	
	char city[100];
	if(fgets(city, 100, client) == NULL)
	{
	
		fprintf(stderr, "Client Text File empty.\n");
		exit(1);
	}
	
	//remove client.txt newline
	size_t ln = strlen(city) - 1;
	if (city[ln] == '\n')
	{
		city[ln] = '\0';
	}
	
	int i;
	i= searchDataBase(city);
	if(i== -1)
	{
		fprintf(stderr, "City not found in database.\n");
		exit(1);
	}
	
	//Write the details to the output result file.
	char outputpath[512];
	strcpy(outputpath, path);
	strcat(outputpath, ".result");
	FILE *output;
	output = fopen(outputpath,"w+");
  
    fprintf(output, "%s : %s,%s,%s\n", DataBase[i].city, DataBase[i].word1, DataBase[i].word2, DataBase[i].word3);
	
	if(fclose(output) != 0)
	{
		perror("Error 2");
		exit(1);
	}
}

//Finds the index of the city in the database.
int searchDataBase(char *city)
{
	int i;
	for(i = 0; i < dbSize; i++)
	{
		if((strcmp(city, DataBase[i].city)) == 0)
		{
			return i;
		}
	}
	return -1;
}

//add client.txt file to the end of the shared queue.
void addNode()
{
	if(head == NULL)
	{
		head = (node_t*)malloc(sizeof(node_t));
		head->path = (char*)malloc(sizeof(clientPath[clientIdx]));
		strcpy(head->path, clientPath[clientIdx]);
		free(clientPath[clientIdx]);
		tail = head;
		tail->next = NULL;
	}
	else
	{
		tail->next = (node_t*)malloc(sizeof(node_t));
		tail = tail->next;
		tail->next = NULL;
		tail->path = (char*)malloc(sizeof(clientPath[clientIdx]));
		strcpy(tail->path, clientPath[clientIdx]);
		free(clientPath[clientIdx]);
	}		
	clientIdx++;
}

//prints paths in nodes, used for debugging.
void printNodes()
{
	node_t* temp = head;
	while(temp != NULL)
	{
		fprintf(stderr,"node: %s\n", temp->path);
		temp = temp->next;
	}
}

//returns file path in queue, removes node from queue.
node_t* popNode()
{
	node_t *node;
	
	if(head == NULL)
	{
		fprintf(stderr,"Pop attempted when there are no nodes in the queue.\n");
		exit(1);
	}
	else
	{
		node = head;
		head = head->next;
	}
	return node;
}
