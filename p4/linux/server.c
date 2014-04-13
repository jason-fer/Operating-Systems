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
char *schedalg = "Must be FIFO, SNFNF or SFF";
request_buffer *buffer; // a list of connection file descriptors
volatile int buffers = 0; // max buffers
volatile int numfull = 0; // number of items currently in the buffer
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
void getargs(int *port, int *threads, volatile int *buffers, char **schedalg, int argc, char *argv[])
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

int 
compareKeys(const void *r1, const void *r2)
{
  request_buffer *bf1 = (request_buffer*) r1;
  request_buffer *bf2 = (request_buffer*) r2;
  int parm1 = 0;
  int parm2 = 0;

	if(strcmp("FIFO", schedalg) == 0)
	{
		parm1 = bf1->filesize;
		parm2 = bf2->filesize;
	} 
	else if(strcmp("SFNF", schedalg) == 0)
	{
		// Smallest filename first.
		parm1 = strlen(bf1->filename);
		parm2 = strlen(bf2->filename);
	} 
	else if(strcmp("SFF", schedalg) == 0)
	{
		// Smallest file first
		parm1 = bf1->filesize;
		parm2 = bf2->filesize;
	}

	// Reverse sorting order (-1 / 1 switched)
  if(parm1 > parm2) {
    return -1;
  } else if(parm1 < parm2) {
    return 1;
  }

  return 0;
}

void bufferDump()
{
	int i;
	// Dump the buffer data
	for (i = 0; i < numfull; i++)
	{
		printf("buffer[%d].connfd %d\n", i, buffer[i].connfd);
		printf("buffer[%d].filesize %d\n", i, buffer[i].filesize);
		printf("buffer[%d].filename %s\n", i, buffer[i].filename);
		printf("buffer[numfull].filename %s\n", buffer[numfull].filename);
	}
	// exit(1);
}

/**
 * Sort the contents of the buffer
 */
void
sortQueue()
{
	// Policies:
	// -First-in-First-out (FIFO)
	// -Smallest Filename First (SFNF)
	// -Smallest File First (SFF)
  qsort(buffer, numfull, sizeof(request_buffer), compareKeys);
}

/**
 * Fill the queue with HTTP requests. A function for the master (producer) 
 * thread. Requires a port number > 0
 */
void *producer_fill(void *portnum)
{
	int *port = (int*) portnum;
	// printf("(int) port: %d\n", *port);
	int listenfd, clientlen;
	listenfd = Open_listenfd(*port); // This opens a socket & listens on port #
	struct sockaddr_in clientaddr;

	while (1) {
		pthread_mutex_lock(&m);
		// printf("PRODUCER HAVE LOCK\n");
		while (numfull == buffers){
			// printf("PRODUCER WAIT\n");
			pthread_cond_wait(&empty, &m);

		}
		clientlen = sizeof(clientaddr);
		buffer[numfull].connfd = Accept(listenfd, (SA *)&clientaddr, (socklen_t *) &clientlen);
		// 
		// CS537: In general, don't handle the request in the main thread.
		// Save the relevant info in a buffer and have one of the worker threads 
		// do the work. However, for SFF, you may have to do a little work
		// here (e.g., a stat() on the filename) ...
		//
		queueRequest(&buffer[numfull]);
		numfull++;
		pthread_cond_signal(&fill);
		// printf("PRODUCER RELEASE LOCK\n");
		pthread_mutex_unlock(&m);
		// requestHandle(&buffer[numfull]);
		// Close(connfd);
	}
}

/**
 * Function for the consumers in the threadpool
 */
void *consumer_empty()
{
	while (1) {
		pthread_mutex_lock(&m);
		// printf("CONSUMEr HAVE LOCK\n");
		// wait if buffer is empty
		while (numfull == 0){
			// printf("CONSUMEr WAIT\n");
			pthread_cond_wait(&fill, &m);
		}
		sortQueue(); //bufferDump();
		// if(numfull == buffers) sortQueue(); 
		// bufferDump();
		// Fulfill requests from queue in order of scheduling policy
		requestHandle(&buffer[numfull - 1]);
		Close(buffer[numfull - 1].connfd);
		// wipe out item in buffer ?
		// Decrement the number of requests in the queue
		numfull--;
		pthread_cond_signal(&empty);
		// printf("CONSUMEr RELEASE LOCK\n");
		pthread_mutex_unlock(&m);
	}
}

int main(int argc, char *argv[])
{
	int port, threads, i;
	/**
	 *  master thread (producer): responsible for accepting new http connections 
	 *  over the network and placing the descriptor for this connection into a 
	 *  fixed-size buffer
	 *  -cond var: block & wait if buffer is full
	 */
	getargs(&port, &threads, &buffers, &schedalg, argc, argv);
	// printf("port:%d, threads:%d, buffers:%d, (did it work?) schedalg:%s \n", port, threads, buffers, schedalg);
	buffer = (request_buffer *) malloc(buffers * sizeof(request_buffer));

	pthread_t pid, cid[threads];
	// Init the master (producer)
	pthread_create(&pid, NULL, producer_fill, (void*) &port);
	for (i = 0; i < threads; i++)
	{
		// Init the worker thread pool
		// child threads (start w/ one for simplicity)
		pthread_create(&cid[i], NULL, consumer_empty, NULL);
	}

	pthread_join(pid, NULL);
	// Create a fixed size pool of worker (consumer) threads.
	for (i = 0; i < threads; i++)
	{
		// Init the worker thread pool
		// child threads (start w/ one for simplicity)
		pthread_join(cid[i], NULL);
	}

	return 0;
}
