#include <sys/shm.h>
#include <sys/msg.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "msg.h" /* FOr the message struct */

/* The size of the memory chunk */
#define SHARED_MEMORY_CHUNK_SIZE 1000;

int shmid; /* The shared memory ID */
pid_t recvPid; /* The receiver process ID */
void* sharedMemPtr; /* The pointer to the shared memory */

void init(int&, void*&);
void retrievePID(int&, const char*);
void cleanUp(const int&, void*);
void send(const char*);

int main(int argc, char** argv)
{
	/* Check the command line arguments */
	if(argc < 2)
	{
		fprintf(stderr, "USAGE: %s <FILE NAME>\n", argv[0]);
		exit(-1);
	}
	/* Connect to shared memory send process ID */
	init(shmid, sharedMemPtr);
	/* Retrieve the reciever's PID and wait for SIGUSR1 */
	retrievePID(recvPid, argv[1]);
	/* Send the file */
	send(argv[1]);
	/* Cleanup */
	cleanUp (shmid, sharedMemPtr);
	return 0;
}

void init(int& shmid, void*& sharedMemPtr)
{
	key_t key;
	/* Use ftok with keyfile.txt to create a unique key */
	if((key = ftok("keyfile.txt", 'a')) == -1)
	{
		perror("ftok");
		exit(-1);
	}
	/* Allocate a shared memory chunk */
	if((shmid = shmget(key, SHARED_MEMORY_CHUNK_SIZE, 0666)) < 0)
	{
		perror("shmget");
		exit(-1);
	}
	/* Retrieve shared memory pointer and attach to shared memory */
	if((sharedMemPtr = shmat(shmid, NULL, 0) == (char*)-1)
	{
		perror("shmat");
		exit(-1);
	}
}

void retrievePID(int& recvPid, const char* fileName)
{
	/* Retrieve the receiver PID from the shared memory*/
	recvPid = *((int*)sharedMemPtr);
	/* Write the sender PID into shared memory */
	*((int*)sharedMemPtr) = getpid();
	/* Send the SIGUSR1 signal to the receiver */
	kill(recvPid, SIGUSR1);
	/* Wait for SIGUSR1 to begin reading the file */
	signal(SIGUSR1,send(fileName));
	while(TRUE){};
}

void cleanUp(const int& shmid, void* sharedMemPtr)
{
	/* Detach from shared memory */
	shmdt(sharedMemPtr);
}

/* The main send function */
void send(const char* fileName)
{	int size;
	/* Open the file for reading */
	FILE *fp = fopen(fileName, "r");
	/* Was the file open? */
	if(!fp)
	{
		perror("fopen");
		exit(-1);
	}
	/* Read the entire file */
	while(!feof(fp))
	{
		if(size = fread(sharedMemPtr+4, sizeof(char), SHARED_MEMORY_CHUNK_SIZE-4, fp)) < 0)
		{
			perror("fread");
			exit(-1);
		}
		/* Write the size into the first four bytes of the shared memory */
		*((int*)sharedMemPtr) = size;
		/* Let the receiver know that it can start reading */
		kill(recvPid, SIGUSR1);
		/* Wait for SIGUSR1 from receiver to continue
		   writing into shared memory */
		signal(SIGUSR1,SIGCONT);
		/* Pause the reading */
		pause();
	}
	/* Close the file */
	fclose(fp);
}
