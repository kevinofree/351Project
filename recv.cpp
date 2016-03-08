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
int shmid, msqid;

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

void init(int& shmid, int& msqid, void*& sharedMemPtr)
{
	key_t key;
	if((key = ftok("keyfile.txt",'a')) == -1)//Use ftok("keyfile.txt", 'a') in order to generate the key.
	{
		perror("ftok");
		exit(1);
	}
	//printf("%d\n", key);

	if((msqid = msgget(key, 0)) == -1)//create message que
	{
		perror("msgget");
		exit(1);
	}
	printf("%d\n", msqid);

	if((shmid = shmget(key, SHARED_MEMORY_CHUNK_SIZE, 0666)) < 0)//allocate shared memory location
	{
		perror("shmget");
		exit(1);
	}
	printf("%d\n", shmid);

	if((sharedMemPtr = shmat(shmid, NULL, 0)) == (char *)-1)//attach shared mem ptr to shared memory location
	{
		perror("shmat");
		exit(1);
	}
	printf("%p\n",sharedMemPtr);
}

/**
 * The main loop
 */
void mainLoop()
{
	message msg;
	/* The size of the mesage */
	int msgSize = 0;
	/* Open the file for writing */
	FILE* fp = fopen(recvFileName, "w");
	/* Error checks */
	if(!fp)
	{
		perror("fopen");
		exit(-1);
	}

	/* Keep receiving until the sender set the size to 0, indicating that
 	 * there is no more data to send
 	 */
	do
	{
		if(msgrcv(msqid, &msg, sizeof(int), 1, 0) != -1)//recieve message from the queue
		{
			msgSize = msg.size;
			printf("message size = %d\n", msgSize);
		}
		else
		{
			perror("msgrcv");
			exit(-1);
		}
		/* If the sender is not telling us that we are done, then get to work */
		if(msgSize != 0)
		{
			/* Save the shared memory to file */
			if(fwrite(sharedMemPtr, sizeof(char), msgSize, fp) < 0)
				perror("fwrite");
			msg.mtype = 2;
			msg.size = 0;
			msgsnd(msqid, &msg, sizeof(int), 0);//tell the sender we are ready for another message
		}
		/* We are done */
		else
			fclose(fp);/* Close the file */

	}while(msgSize != 0);
}
/**
 * Perfoms the cleanup functions
 * @param sharedMemPtr - the pointer to the shared memory
 * @param shmid - the id of the shared memory segment
 * @param msqid - the id of the message queue
 */

void cleanUp(const int& shmid, const int& msqid, void* sharedMemPtr)
{
	/* Detach from shared memory */
	shmdt(sharedMemPtr);
	/* Deallocate the shared memory chunk */
	shmctl(shmid, IPC_RMID, NULL);
	/* Deallocate the message queue */
	msgctl(msqid, IPC_RMID, NULL);
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
