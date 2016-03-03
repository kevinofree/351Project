#include <sys/shm.h>
#include <sys/msg.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "msg.h"    /* For the message struct */

/* The size of the shared memory chunk */
#define SHARED_MEMORY_CHUNK_SIZE 1000

/* The ids for the shared memory segment and the message queue */
int shmid, msqid;

/* The pointer to the shared memory */
void* sharedMemPtr;

/**
 * Sets up the shared memory segment and message queue
 * @param shmid - the id of the allocated shared memory 
 * @param msqid - the id of the shared memory
 */

void init(int& shmid, int& msqid, void*& sharedMemPtr)
{
	/* TODO: 
        1. Create a file called keyfile.txt containing string "Hello world" (you may do
 		    so manually or from the code).
	    2. Use ftok("keyfile.txt", 'a') in order to generate the key.
		3. Use the key in the TODO's below. Use the same key for the queue
		    and the shared memory segment. This also serves to illustrate the difference
		    between the key and the id used in message queues and shared memory. The id
		    for any System V objest (i.e. message queues, shared memory, and sempahores) 
		    is unique system-wide among all SYstem V objects. Two objects, on the other hand,
		    may have the same key.
	 */ // check
	

	
	/* TODO: Get the id of the shared memory segment. The size of the segment must be SHARED_MEMORY_CHUNK_SIZE */ // check
	/* TODO: Attach to the shared memory */ // check
	/* TODO: Attach to the message queue */ // check
	/* Store the IDs and the pointer to the shared memory region in the corresponding parameters */ //check
	key_t key; //initialize keys
	key = ftok("keyfile.txt", 'a');
	
	shmid = shmget(key, SHARED_MEMORY_CHUNK_SIZE, 0644 | IPC_CREAT); //creating or checking if the shared memory location exists
	
	msqid = msgget(key, 0666 | IPC_CREAT); //creating or checking if the message queue exists
	
	sharedMemPtr = shmat(shmid, (void *)0, 0);	//assigning the location of shared memory to a void pointer
}

/**
 * Performs the cleanup functions
 * @param sharedMemPtr - the pointer to the shared memory
 * @param shmid - the id of the shared memory segment
 * @param msqid - the id of the message queue
 */

void cleanUp(const int& shmid, const int& msqid, void* sharedMemPtr)
{
	/* TODO: Detach from shared memory */
	shmdt(sharedMemPtr);//Detatch from shared memory
	shmctl(shmid, IPC_RMID, NULL);//Destroy the shared memory location
	msgctl(msqid, IPC_RMID, NULL);//Destory the message que
}

/**
 * The main send function
 * @param fileName - the name of the file
 */
void send(const char* fileName)
{
	/* Open the file for reading */
	FILE* fp = fopen(fileName, "r");
	int size = sizeof(struct message) - sizeof(long);


	/* A buffer to store message we will send to the receiver. */
	message sndMsg;
	sndMsg.mtype = SENDER_DATA_TYPE;//not sure if needed

	/* A buffer to store message received from the receiver. */
	message rcvMsg;
	//rcvMsg.mtype = RECV_DONE_TYPE;//not sure if needed

	/* Was the file open? */
	if(!fp)
	{
		perror("fopen");
		exit(-1);
	}

	/* Read the whole file */
	while(!feof(fp))
	{
		/* Read at most SHARED_MEMORY_CHUNK_SIZE from the file and store them in shared memory. 
 		 * fread will return how many bytes it has actually read (since the last chunk may be less
 		 * than SHARED_MEMORY_CHUNK_SIZE).
 		 */
		if((sndMsg.size = fread(sharedMemPtr, sizeof(char), SHARED_MEMORY_CHUNK_SIZE, fp)) < 0)
		{
			perror("fread");
			exit(-1);
		}

		/* TODO: Send a message to the receiver telling him that the data is ready 
 		 * (message of type SENDER_DATA_TYPE)
 		 */
		msgsnd(msqid, &sndMsg, size, 0);


		/* TODO: Wait until the receiver sends us a message of type RECV_DONE_TYPE telling us 
 		 * that he finished saving the memory chunk.
 		 */
 		do{
			msgrcv (msqid, &rcvMsg, size, 0);
 		}while(rcvMsg.mtype != RECV_DONE_TYPE);//wait until we receive proper notification from receiver
 		rcvMsg.mtype = 0;//clear receive message
	}


	/* TODO: once we are out of the above loop, we have finished sending the file.
 	  * Lets tell the receiver that we have nothing more to send. We will do this by
 	  * sending a message of type SENDER_DATA_TYPE with size field set to 0.
	  */
	sndMsg.size = 0;
	msgsnd(msqid, &sndMsg, size, 0);

	/* Close the file */
	fclose(fp);

}



int main(int argc, char** argv)
{

	/* Check the command line arguments */
	if(argc < 2)
	{
		fprintf(stderr, "USAGE: %s <FILE NAME>\n", argv[0]);
		exit(-1);
	}

	/* Connect to shared memory and the message queue */
	init(shmid, msqid, sharedMemPtr);

	/* Send the file */
	send(argv[1]);

	/* Cleanup */
	cleanUp(shmid, msqid, sharedMemPtr);

	return 0;
}
