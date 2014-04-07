#include "cs537.h"
#include "request.h"

// 
// server.c: A very, very simple web server
//
// To run:
//  server <portnum (above 2000)>
//
// Repeatedly handles HTTP requests sent to this port number.
// Most of the work is done within routines written in request.c
//

/**
 * Usage: 
 * prompt> server [portnum] [threads] [buffers] [schedalg]
 * 
 * -portnum: the port number to listen on
 * -threads: size  of server worker thread pool
 * -buffers: # of request connections that can be accepted @ one time. 
 * -schedalg: must be FIFO, SFNF or SFF
 * (threads & buffers must both be > 0)
 * @todo: parse the new arguments
 */
void getargs(int *port, int argc, char *argv[])
{
	if (argc != 2) {
		// prompt> server [portnum] [threads] [buffers] [schedalg]
		fprintf(stderr, "Usage: %s <port>\n", argv[0]);
		exit(1);
	}
	*port = atoi(argv[1]);
}


int main(int argc, char *argv[])
{
	int listenfd, connfd, port, clientlen;
	struct sockaddr_in clientaddr;

	/**
	 *  master thread (producer): responsible for accepting new http connections 
	 *  over the network and placing the descriptor for this connection into a 
	 *  fixed-size buffer
	 *  -cond var: block & wait if buffer is full
	 *  
	 *  Policies:
	 *  -First-in-First-out (FIFO)
	 *  -Smallest Filename First (SFNF)
	 *  -Smallest File First (SFF)
	 */
	getargs(&port, argc, argv);

	/**
	 * Create a fixed size pool of worker (consumer) threads.
	 * -cond var: wait if buffer is empty
	 * On Wake:
	 * -pull requests from queue in order of scheduling policy
	 * -read network descriptor, & run requestServeDynamic or 
	 * requestServeStatic
	 */
	
	/**
	 * Commands to use:
	 * thread_create, pthread_mutex_init, pthread_mutex_lock, pthread_mutex_unlock, 
	 * pthread_cond_init, pthread_cond_wait, pthread_cond_signal
	 */
	
	listenfd = Open_listenfd(port); // This opens a socket & listens on port #
	while (1) {
		clientlen = sizeof(clientaddr);
		connfd = Accept(listenfd, (SA *)&clientaddr, (socklen_t *) &clientlen);

		// 
		// CS537: In general, don't handle the request in the main thread.
		// Save the relevant info in a buffer and have one of the worker threads 
		// do the work. However, for SFF, you may have to do a little work
		// here (e.g., a stat() on the filename) ...
		// 
		requestHandle(connfd);

		Close(connfd);
	}

}


	


 
