#include "cs537.h"
#include "request.h"
/**
 * server.c: A simple web server
 *
 * Repeatedly handles HTTP requests sent to the port number.
 * Most of the work is done within routines written in request.c
 *
 * Generates a threadpool and buffer size based on specified sizes (both must)
 * be > 0. 
 */
char *schedalg = "Must be FIFO, SNFNF or SFF";
request_buffer *buffer; // a list of connection file descriptors
pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t empty = PTHREAD_COND_INITIALIZER;
pthread_cond_t fill = PTHREAD_COND_INITIALIZER;
int FIFO = 0;
// Simple Queue (buffers ==  max buffers / items)
int buffers = 0; // max buffers
int numitems = 0; // number of items currently in the buffer
int frontIndex = 0;
int rearIndex = -1;

// Start Pseudo-Queue functions: -->
int incrementIndex(int index){
	// Length = number of components in the array
	if (index == buffers - 1){ // was items.length
		return 0;
	} else {
		return index + 1;
	}
}

// Returns index to add the item
int enqueueIndex() {

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
int dequeueIndex() {
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
	*port = atoi(argv[1]);
}

int compareKeys(const void *r1, const void *r2)
{
  request_buffer *bf1 = (request_buffer*) r1;
  request_buffer *bf2 = (request_buffer*) r2;
  int parm1 = 0;
  int parm2 = 0;

	if(strcmp("SFNF", schedalg) == 0)
	{
		parm1 = strlen(bf1->filename);
		parm2 = strlen(bf2->filename);
	} 
	else if(strcmp("SFF", schedalg) == 0)
	{
		parm1 = bf1->filesize;
		parm2 = bf2->filesize;
	}

	// Reverse sorting order (-1 / 1 switched); ASC.
  if(parm1 > parm2) {
    return -1;
  } else if(parm1 < parm2) {
    return 1;
  }

  return 0;
}

void currBufferDump()
{
	int i = numitems - 1;
	printf("buffer[%d].connfd %d\n", i, buffer[i].connfd);
	printf("buffer[%d].filesize %d\n", i, buffer[i].filesize);
	printf("buffer[%d].filename %s\n", i, buffer[i].filename);
	printf("buffer[numitems].filename %s\n", buffer[i].filename);
}

void bufferDump()
{
	int i;
	for (i = 0; i < numitems; i++)
	{
		printf("buffer[%d].connfd %d\n", i, buffer[i].connfd);
		printf("buffer[%d].filesize %d\n", i, buffer[i].filesize);
		printf("buffer[%d].filename %s\n", i, buffer[i].filename);
		printf("buffer[numitems].filename %s\n", buffer[numitems].filename);
	}
}

int valid_fd(int fd)
{
	return fcntl(fd, F_GETFD) != -1 || errno != EBADF;
}

/**
 * Sort the contents of the buffer
 */
void sortQueue()
{
  qsort(buffer, numitems, sizeof(request_buffer), compareKeys);
}

void *producer_fifo(void *portnum)
{
	int *port = (int*) portnum;
	int listenfd, clientlen, connfd;
	listenfd = Open_listenfd(*port);
	struct sockaddr_in clientaddr;
	int index;
	for (;;) 
	{
		clientlen = sizeof(clientaddr);
		connfd = Accept(listenfd, (SA *)&clientaddr, (socklen_t *) &clientlen);
		pthread_mutex_lock(&m);
		while (numitems == buffers){
			pthread_cond_wait(&empty, &m);
		}
		index = enqueueIndex();
		buffer[index].connfd = connfd;
		pthread_cond_signal(&fill);
		pthread_mutex_unlock(&m);
	}
}

void *consumer_fifo()
{
	int index;
	request_buffer *tmp_buf  = (request_buffer *) malloc(buffers * sizeof(request_buffer));
	for (;;) 
	{
		pthread_mutex_lock(&m);
		while (numitems == 0){
			pthread_cond_wait(&fill, &m);
		}
		index = dequeueIndex();
		tmp_buf->connfd = buffer[index].connfd;
		pthread_cond_signal(&empty);
		pthread_mutex_unlock(&m);
		queueRequest(tmp_buf);
		requestHandle(tmp_buf); 
		Close(tmp_buf->connfd);
	}
}

/**
 * Fill the queue with HTTP requests. A function for the master (producer) 
 * thread. Requires a port number > 0
 * master thread (producer): responsible for accepting new http connections 
 *  over the network and placing the descriptor for this connection into a 
 *  fixed-size buffer
 *  -cond var: block & wait if buffer is full
 */
void *producer(void *portnum)
{
	int *port = (int*) portnum;
	int listenfd, clientlen, connfd;
	listenfd = Open_listenfd(*port);
	struct sockaddr_in clientaddr;
	int index;
	for (;;) 
	{
		// Hold locks for the shortest time possible.
		clientlen = sizeof(clientaddr);
		// This blocks; must be outside mutex
		connfd = Accept(listenfd, (SA *)&clientaddr, (socklen_t *) &clientlen);
		// No point hanging on to this connection if it's broken
		// if(connfd <= 0 || !valid_fd(connfd)) continue;
		pthread_mutex_lock(&m);
		while (numitems == buffers){
			pthread_cond_wait(&empty, &m);
		}
		index = enqueueIndex();
		buffer[index].connfd = connfd;
		if(! FIFO){
			queueRequest(&buffer[index]);
			// There's no point sorting unless we have at least two items
			if(numitems > 1) sortQueue();
		}
		pthread_cond_signal(&fill);
		pthread_mutex_unlock(&m);
	}
}

/**
 * Function for the consumers in the threadpool
 */
void *consumer()
{
	int index;
	request_buffer *tmp_buf  = (request_buffer *) malloc(buffers * sizeof(request_buffer));
	for (;;) 
	{
		pthread_mutex_lock(&m);
		while (numitems == 0){
			pthread_cond_wait(&fill, &m);
		}
		index = dequeueIndex();
		// Is there a faster way for SFF & SFNF?
		// Store data in temp struct (while locked)
		tmp_buf->filesize = buffer[index].filesize;
		tmp_buf->connfd = buffer[index].connfd;
		tmp_buf->is_static = buffer[index].is_static;
		strcpy(tmp_buf->buf, buffer[index].buf);
		strcpy(tmp_buf->method, buffer[index].method);
		strcpy(tmp_buf->uri, buffer[index].uri);
		strcpy(tmp_buf->version, buffer[index].version);
		strcpy(tmp_buf->filename, buffer[index].filename);
		strcpy(tmp_buf->cgiargs, buffer[index].cgiargs);
		tmp_buf->sbuf = buffer[index].sbuf;
		tmp_buf->rio = buffer[index].rio;
		// Let go of lock as quickly as possible...
		pthread_cond_signal(&empty);
		pthread_mutex_unlock(&m);
		// // Multi-thread the queueRequest work if we are in FIFO mode.
		// if(FIFO) queueRequest(tmp_buf);
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
	pthread_t pid, cid[threads];

	if(! FIFO){
		// Init producer
		pthread_create(&pid, NULL, producer, (void*) &port);
		// Create fixed size pool of consumer threads.
		for (i = 0; i < threads; i++)
		{
			pthread_create(&cid[i], NULL, consumer, NULL);
		}
	} else {
		// Init producer
		pthread_create(&pid, NULL, producer_fifo, (void*) &port);
		// Create fixed size pool of consumer threads.
		for (i = 0; i < threads; i++)
		{
			pthread_create(&cid[i], NULL, consumer_fifo, NULL);
		}	
	}

	pthread_join(pid, NULL);
	for (i = 0; i < threads; i++)
	{
		pthread_join(cid[i], NULL);
	}

	return 0;
}