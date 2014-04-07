#include "cs537.h"
#include "request.h"
#include "pthread.h"

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
 * Function for the master (producer) thread
 * Requires a port number > 0
 */
void *producer_listen(void *portnum)
{
	int *port = (int*) portnum;
	// printf("(int) port: %d\n", *port);
	// exit(1);
	int listenfd, connfd, clientlen;
	listenfd = Open_listenfd(*port); // This opens a socket & listens on port #
	struct sockaddr_in clientaddr;

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

int main(int argc, char *argv[])
{
	int port, threads, buffers;
	
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
	printf("port:%d, threads:%d, buffers:%d, (did it work?) schedalg:%s \n", port, threads, buffers, schedalg);

	/**
	 * Create a fixed size pool of worker (consumer) threads.
	 * -cond var: wait if buffer is empty
	 * On Wake:
	 * -pull requests from queue in order of scheduling policy
	 * -read network descriptor, & run requestServeDynamic or 
	 * requestServeStatic
	 */
	// producer_listen((void*) &port);
	pthread_t pid;//, cid[threads];
	pthread_create(&pid, NULL, producer_listen, (void*) &port);
	pthread_join(pid, NULL);
	
	// // for (int i = 0; i < threads; i++)
	// // {
	// // 	// int pthread_create(&pid, NULL, producer_listen, &port);		 
 // // 		 // int pthread_create(pthread_t *thread, const pthread_attr_t *attr, void *(*start_routine) (void *), void *arg);
	// // }


	/**
	 * Commands to use:
	 * thread_create, pthread_mutex_init, pthread_mutex_lock, pthread_mutex_unlock, 
	 * pthread_cond_init, pthread_cond_wait, pthread_cond_signal
	 */
	


}


	


 
