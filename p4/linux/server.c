#include "cs537.h"
#include "request.h"

/**
 * server.c: A very, very simple web server
 *
 * Repeatedly handles HTTP requests sent to the port number.
 * Most of the work is done within routines written in request.c
 *
 * Generates a threadpool and buffer size based on specified sizes (both must)
 * be > 0. 
 */
request_buffer *buffer; // a list of connection file descriptors
int buffers = 0; // max buffers
int numfull = 0; // number of items currently in the buffer
pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t empty = PTHREAD_COND_INITIALIZER;
pthread_cond_t fill = PTHREAD_COND_INITIALIZER;

/**
 * Usage: 
 * prompt> ./server [portnum] [threads] [buffers] [schedalg]
 * 
 * -portnum: the port number to listen on (above 2000)
 * -threads: size  of server worker thread pool
 * -buffers: # of request connections that can be accepted @ one time. 
 * -schedalg: must be FIFO, SFNF or SFF
 * (threads & buffers must both be > 0)
 * @todo: parse the new arguments
 */
void getargs(int *port, int *threads, int *buffers, char **schedalg, int argc, char *argv[])
{
	if (argc != 5 // Must have 5x arguments
			|| (*threads = atoi(argv[2])) < 1 // Must have 1 or more threads
			|| (*buffers = atoi(argv[3])) < 1 // Must have 1 or more buffers
			|| ( // <schedalg> must be FIFO, SFNF, or SFF
					strcmp("FIFO", (*schedalg = strdup(argv[4]))) != 0
					&& strcmp("SFNF", *schedalg) != 0 
					&& strcmp("SFF", *schedalg) != 0 
					)
			) {
		// prompt> server [portnum] [threads] [buffers] [schedalg]
		fprintf(stderr, "Usage: %s <portnum> <threads> <buffers> <schedalg>\n", argv[0]);
		exit(1);
	} 

	*port = atoi(argv[1]);
}

/**
 * Fill the queue with HTTP requests. A function for the master (producer) 
 * thread. Requires a port number > 0
 */
void *producer_fill(void *portnum)
{
	int *port = (int*) portnum;
	// printf("(int) port: %d\n", *port);
	// exit(1);
	int listenfd, connfd, clientlen;
	listenfd = Open_listenfd(*port); // This opens a socket & listens on port #
	struct sockaddr_in clientaddr;

	while (1) {
		pthread_mutex_lock(&m);
		if (numfull == buffers){
			// Dump the buffer data
			if(numfull == buffers){
				for (int i = 0; i < buffers; i++)
				{
					printf("buffer[%d].connfd %d\n", i, buffer[i].connfd);
					printf("buffer[%d].filesize %d\n", i, buffer[i].filesize);
					printf("buffer[%d].filename %s\n", i, buffer[i].filename);
				}
				exit(1);
			}
			pthread_cond_wait(&empty, &m);
		}
		clientlen = sizeof(clientaddr);
		connfd = Accept(listenfd, (SA *)&clientaddr, (socklen_t *) &clientlen);
		// 
		// CS537: In general, don't handle the request in the main thread.
		// Save the relevant info in a buffer and have one of the worker threads 
		// do the work. However, for SFF, you may have to do a little work
		// here (e.g., a stat() on the filename) ...
		//
		queueRequest(&buffer[numfull], connfd);
		numfull++;
		pthread_cond_signal(&fill);
		pthread_mutex_unlock(&m);
		// requestHandle(connfd);
		// Close(connfd);
	}
}

/**
 * Function for the consumers in the threadpool
 */
void *consumer_empty()
{
	while (1) {
		/*
		pthread_mutex_lock(&m);
		if (numfull == 0){
			pthread_cond_wait(&fill, &m);
		}
		// Fulfill request
		requestHandle(buffer[numfull].connfd);
		Close(buffer[numfull].connfd);
		dequeueRequest(&buffer[numfull]);
		// Make sure to decrement the number of requests in the queue so we don't
		// fullfill the same request twice....
		numfull--;
		pthread_cond_signal(&empty);
		pthread_mutex_unlock(&m);
		*/
	}
}

int main(int argc, char *argv[])
{
	int port, threads;
	// Policies:
	// -First-in-First-out (FIFO)
	// -Smallest Filename First (SFNF)
	// -Smallest File First (SFF)
	char *schedalg = "Must be FIFO, SNFNF or SFF";
	/**
	 *  master thread (producer): responsible for accepting new http connections 
	 *  over the network and placing the descriptor for this connection into a 
	 *  fixed-size buffer
	 *  -cond var: block & wait if buffer is full
	 */
	getargs(&port, &threads, &buffers, &schedalg, argc, argv);
	// printf("port:%d, threads:%d, buffers:%d, (did it work?) schedalg:%s \n", port, threads, buffers, schedalg);
	buffer = (request_buffer *) malloc(buffers * sizeof(request_buffer));

	// Init the buffer
	for (int i = 0; i < buffers; i++)
	{
		buffer[i].filesize = 0;
		buffer[i].connfd = 0;
		buffer[i].filename = strdup("");
	}

	pthread_t pid, cid[threads];
	// Init the master (producer)
	pthread_create(&pid, NULL, producer_fill, (void*) &port);
	for (int i = 0; i < threads; i++)
	{
		// Init the worker thread pool
		// child threads (start w/ one for simplicity)
		pthread_create(&cid[0], NULL, consumer_empty, NULL);
		break;
	}

	/**
	 * Create a fixed size pool of worker (consumer) threads.
	 * -cond var: wait if buffer is empty
	 * On Wake:
	 * -pull requests from queue in order of scheduling policy
	 * -read network descriptor, & run requestServeDynamic or 
	 * requestServeStatic
	 */
 	// @stuff todo:
 	// 1-make queueRequest more efficient (remove all the uncessary extra stuff
 	// so we don't get strange problems when fulfilling requests -- i.e., just
 	// get the filename & run fstat & update the buffer, nothing more)
 	// 2-implement each scheduling algorithm to sort correctly (trigger a sort 
 	// once the buffer is full, confirm sorting works as expected) -- use whatever
 	// sorting algorithm I used in fastsort (P1)
 	// 3-confirm things work with just 1 master thread & 1 worker thread
 	// 4-create a pool of worker threads, confirm that things still work
 	// 5-start running the tests

	pthread_join(pid, NULL);
	pthread_join(cid[0], NULL);
}


