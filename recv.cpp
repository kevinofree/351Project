#include <sys/shm.h>
#include <sys/msg.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "msg.h"    /* For the message struct */


/* The size of the shared memory chunk */
#define SHARED_MEMORY_CHUNK_SIZE 1000

/* The ids for the shared memory segment and the message queue */
int shmid, sendPID;

/* The pointer to the shared memory */
void *sharedMemPtr;

/* The name of the received file */
const char recvFileName[] = "recvfile";


/**
 * Sets up the shared memory segment and message queue
 * @param shmid - the id of the allocated shared memory
 * @param msqid - the id of the shared memory
 * @param sharedMemPtr - the pointer to the shared memory
 */

void init(int& shmid, void*& sharedMemPtr)
{
	key_t key;
	if((key = ftok("keyfile.txt",'a')) == -1)//Use ftok("keyfile.txt", 'a') in order to generate the key.
	{
		perror("ftok");
		exit(1);
	}

	if((shmid = shmget(key, SHARED_MEMORY_CHUNK_SIZE, 0666)) < 0)//allocate shared memory location
	{
		perror("shmget");
		exit(1);
	}

	if((sharedMemPtr = shmat(shmid, NULL, 0)) == (char *)-1)//attach shared mem ptr to shared memory location
	{
		perror("shmat");
		exit(1);
	}
}

void retrievePID(FILE* fp)
{
	sendPID = *((int*)sharedMemPtr);
	*((int*)(sharedMemPtr)) = getpid();
	kill (sendPID, SIGUSR1);
	signal (SIGUSR1, retrieveData(fp));
}

void retrieveData(FILE* fp)
{
	int msgSize = *((int*)sharedMemPtr);
	if(msgSize > 0){
		if(fwrite(sharedMemPtr, sizeof(char), msgSize, fp) < 0)
				perror("fwrite");
	}
	else{
		fclose(fp);
		cleanUp();
	}
	
}
/**
 * The main loop
 */
void mainLoop()
{
	FILE* fp = fopen(recvFileName, "w");
	/* Error checks */
	if(!fp)
	{
		perror("fopen");
		exit(-1);
	}
	
	signal(SIGUSR1, retrievePID(fp));
	
	while(true);
}
/**
 * Perfoms the cleanup functions
 * @param sharedMemPtr - the pointer to the shared memory
 * @param shmid - the id of the shared memory segment
 * @param msqid - the id of the message queue
 */

void cleanUp(const int& shmid, void* sharedMemPtr)
{
	/* Detach from shared memory */
	shmdt(sharedMemPtr);
	/* Deallocate the shared memory chunk */
	shmctl(shmid, IPC_RMID, NULL);
	exit();
}

/**
 * Handles the exit signal
 * @param signal - the signal type
 */

void ctrlCSignal(int signal)
{
	/* Free system V resources */
	cleanUp(shmid, msqid, sharedMemPtr);
}

int main(int argc, char** argv)
{
	/* set up ctrlC signal listener */
	signal(SIGINT,ctrlCSignal);
	/* Initialize */
	init(shmid, msqid, sharedMemPtr);
	/* Go to the main loop */
	mainLoop();
	/* Detach from shared memory segment, and deallocate shared memory and message queue (i.e. call cleanup) */
	cleanUp(shmid, msqid, sharedMemPtr);
	return 0;
}
