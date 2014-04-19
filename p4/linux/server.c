#include "cs537.h"
#include "request.h"
/**
 * server.c: Turned a single thread web server into a multi-threaded web server
 *
 * Repeatedly handles HTTP requests sent to the port number.
 * Most of the work is done within routines written in request.c
 *
 * Generates a threadpool and buffer size based on specified sizes (both must)
 * be > 0. 
 */
char *schedalg = "Must be FIFO, SNFNF or SFF";
request_buffer *buffer; // an 'array' of buffer structs
int *fifo_buffer; // a list of connection file descriptors
pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t empty = PTHREAD_COND_INITIALIZER;
pthread_cond_t fill = PTHREAD_COND_INITIALIZER;
int FIFO = 0;
int SFNF = 0;
int SFF = 0;
// Simple Queue (buffers ==  max buffers / items)
int buffers = 0; // max buffers
int numitems = 0; // number of items currently in the buffer
int frontIndex = 0;
int rearIndex = -1;

// Start Pseudo-Queue functions: -->
int incrementIndex(int index){
	return (index + 1) % buffers;
}

// Returns index to add the item
int enqueueIndex(){
	if(FIFO && buffers > 1){
		// Increment numitems & get the right index for a circular array
		rearIndex = incrementIndex(rearIndex);
		numitems++;
	} else {
		// This must be SFF or SFNF, which both need to be sorted.
		rearIndex = numitems;
		numitems++;
	}
	return rearIndex;
}

// Return indexto retrieve the item
int dequeueIndex(){
	int index;
	if(numitems == 0) return -1; // This should never happen
	if(FIFO && buffers > 1){
		index = frontIndex;
		frontIndex = incrementIndex(frontIndex);
		numitems--;
	} else {
		// Make manual adjustments for SFNF, or SFF, or FIFO with 1 buffer
		index = numitems - 1;
		numitems--;
	}
	return index;
}
// <--End Pseudo-Queue functions

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
			|| (
					strcmp("FIFO", (*schedalg = strdup(argv[4]))) != 0
					&& strcmp("SFNF", *schedalg) != 0 
					&& strcmp("SFF",  *schedalg) != 0 
					)
			) {
		// prompt> server [portnum] [threads] [buffers] [schedalg]
		fprintf(stderr, "Usage: %s <portnum> <threads> <buffers> <schedalg>\n", argv[0]);
		exit(1);
	}
	if(strcmp("FIFO", *schedalg) == 0) FIFO = 1;
	if(strcmp("SFNF", *schedalg) == 0) SFNF = 1;
	if(strcmp("SFF", *schedalg) == 0) SFF = 1;
	*port = atoi(argv[1]);
}

int sfnfCompareKeys(const void *r1, const void *r2)
{
  request_buffer *bf1 = (request_buffer*) r1;
  request_buffer *bf2 = (request_buffer*) r2;
  int parm1 = strlen(bf1->filename);
  int parm2 = strlen(bf2->filename);
	// Reverse sorting order (-1 / 1 switched); ASC.
  if(parm1 > parm2) {
    return -1;
  } else if(parm1 < parm2) {
    return 1;
  }
  return 0;
}

int sffCompareKeys(const void *r1, const void *r2)
{
  request_buffer *bf1 = (request_buffer*) r1;
  request_buffer *bf2 = (request_buffer*) r2;
	int parm1 = bf1->filesize;
	int parm2 = bf2->filesize;
	// Reverse sorting order (-1 / 1 switched); ASC.
  if(parm1 > parm2) {
    return -1;
  } else if(parm1 < parm2) {
    return 1;
  }
  return 0;
}

void sfnfSortQueue()
{
  qsort(buffer, numitems, sizeof(request_buffer), sfnfCompareKeys);
}
void sffSortQueue()
{
  qsort(buffer, numitems, sizeof(request_buffer), sffCompareKeys);
}

/**
 * Fill the queue with HTTP requests. A function for the master (producer) 
 * thread. Requires a port number > 0. Responsible for accepting new http 
 * connections over the network and placing the descriptor for this connection 
 * into a fixed-size buffer. Cond var: block & wait if buffer is full
 */
void *producer_fifo(void *portnum)
{
	int *port = (int*) portnum;
	int listenfd, clientlen, connfd;
	listenfd = Open_listenfd(*port);
	struct sockaddr_in clientaddr;
	// Sacrificing code simplicity, for efficiency.
	for (;;) 
	{
		clientlen = sizeof(clientaddr);
		connfd = Accept(listenfd, (SA *)&clientaddr, (socklen_t *) &clientlen);
		pthread_mutex_lock(&m);
		while (numitems == buffers){
			pthread_cond_wait(&empty, &m);
		}
		// Do as little as possible inside the lock
		if(buffers > 1){
			rearIndex = (rearIndex + 1) % buffers;
			numitems++;
			fifo_buffer[rearIndex] = connfd;
		} else {
			rearIndex = numitems;
			numitems++;
			fifo_buffer[rearIndex] = connfd;
		}
		pthread_cond_signal(&fill);
		pthread_mutex_unlock(&m);
	}
}

void *consumer_fifo()
{
	int index;
	int connfd;
	// Sacrificing code simplicity, for efficiency.
	for (;;) 
	{
		// Do as little as possible inside the lock
		pthread_mutex_lock(&m);
		while (numitems == 0){
			pthread_cond_wait(&fill, &m);
		}
		if(buffers > 1){
			// Using circular queue
			index = frontIndex;
			frontIndex = (frontIndex + 1) % buffers;
			numitems--;
			connfd = fifo_buffer[index];
		} else {
			// Using a single buffer
			numitems--;
			connfd = fifo_buffer[numitems];
		}
		pthread_cond_signal(&empty);
		pthread_mutex_unlock(&m);
		// The request is handled almost completely outside the lock... 
		requestHandleFifo(connfd);
		Close(connfd);
	}
}

/**
 * SFNF & SFF producer
 */
void *producer(void *portnum)
{
	int *port = (int*) portnum;
	int listenfd, clientlen, connfd;
	listenfd = Open_listenfd(*port);
	struct sockaddr_in clientaddr;
	request_buffer *tmp_buf  = (request_buffer *) malloc(buffers * sizeof(request_buffer));
	
	for (;;)
	{
		// Hold locks for the shortest time possible.
		clientlen = sizeof(clientaddr);
		// This blocks; must be outside mutex
		connfd = Accept(listenfd, (SA *)&clientaddr, (socklen_t *) &clientlen);
		tmp_buf->connfd = connfd;
		prepRequest(tmp_buf);

		pthread_mutex_lock(&m);
		while (numitems == buffers){
			pthread_cond_wait(&empty, &m);
		}
		rearIndex = numitems;
		numitems++;
		buffer[rearIndex].connfd = tmp_buf->connfd;
		strcpy(buffer[rearIndex].filename, tmp_buf->filename);
		buffer[rearIndex].filesize = tmp_buf->filesize;
		strcpy(buffer[rearIndex].buf, tmp_buf->buf);
		buffer[rearIndex].rio = tmp_buf->rio;
		// if(numitems > 1){
			if(SFNF){
				qsort(buffer, numitems, sizeof(request_buffer), sfnfCompareKeys);
			} else { // SFF
				qsort(buffer, numitems, sizeof(request_buffer), sffCompareKeys);
			}
		// }
		pthread_cond_signal(&fill);
		pthread_mutex_unlock(&m);
	}
}

void *consumer()
{
	request_buffer *tmp_buf  = (request_buffer *) malloc(buffers * sizeof(request_buffer));
	for (;;) 
	{
		pthread_mutex_lock(&m);
		while (numitems == 0){
			pthread_cond_wait(&fill, &m);
		}
		numitems--;
		strcpy(tmp_buf->buf, buffer[numitems].buf);
		strcpy(tmp_buf->filename, buffer[numitems].filename);
		tmp_buf->connfd = buffer[numitems].connfd;
		tmp_buf->rio = buffer[numitems].rio;
		pthread_cond_signal(&empty);
		pthread_mutex_unlock(&m);
		requestHandle(tmp_buf); // Note: empty file crashes server.
		Close(tmp_buf->connfd);
	}
}

int main(int argc, char *argv[])
{
	int port, threads, i;
	getargs(&port, &threads, &buffers, &schedalg, argc, argv);

	// printf("port:%d, threads:%d, buffers:%d, (did it work?) schedalg:%s \n", port, threads, buffers, schedalg);
	buffer = (request_buffer *) malloc(buffers * sizeof(request_buffer));
	fifo_buffer = (int *) malloc(buffers * sizeof(int));
	pthread_t pid, cid[threads];

	// Init producer & create fixed size pool of consumer threads.
	if(FIFO){
		pthread_create(&pid, NULL, producer_fifo, (void*) &port);
		for (i = 0; i < threads; i++){
			pthread_create(&cid[i], NULL, consumer_fifo, NULL);
		}
	} else {
		pthread_create(&pid, NULL, producer, (void*) &port);
		for (i = 0; i < threads; i++){
			pthread_create(&cid[i], NULL, consumer, NULL);
		}
	}

	pthread_join(pid, NULL);
	for (i = 0; i < threads; i++)
	{
		pthread_join(cid[i], NULL);
	}

	return 0;
}