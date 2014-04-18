/*
 * client.c: A very, very primitive HTTP client.
 * 
 * To run, try: 
 *      client www.cs.wisc.edu 80 /
 *
 * Sends one HTTP request to the specified HTTP server.
 * Prints out the HTTP response.
 *
 * CS537: For testing your server, you will want to modify this client.  
 * For example:
 * 
 * You may want to make this multi-threaded so that you can 
 * send many requests simultaneously to the server.
 *
 * You may also want to be able to request different URIs; 
 * you may want to get more URIs from the command line 
 * or read the list from a file. 
 *
 * When we test your server, we will be using modifications to this client.
 *
 */

#include "cs537.h"

	char *host, *filename;
	int port;

/*
 * Send an HTTP request for the specified file 
 */
void clientSend(int fd, char *filename)
{
	char buf[MAXLINE];
	char hostname[MAXLINE];

	Gethostname(hostname, MAXLINE);

	/* Form and send the HTTP request */
	sprintf(buf, "GET %s HTTP/1.1\n", filename);
	sprintf(buf, "%shost: %s\n\r\n", buf, hostname);
	Rio_writen(fd, buf, strlen(buf));
}
	
/*
 * Read the HTTP response and print it out
 */
void clientPrint(int fd)
{
	rio_t rio;
	char buf[MAXBUF];  
	// int length = 0;
	int n;
	
	Rio_readinitb(&rio, fd);

	/* Read and display the HTTP Header */
	n = Rio_readlineb(&rio, buf, MAXBUF);
	while (strcmp(buf, "\r\n") && (n > 0)) {
		printf("Header: %s", buf);
		// n = Rio_readlineb(&rio, buf, MAXBUF);

		/* If you want to look for certain HTTP tags... */
		// if (sscanf(buf, "Content-Length: %d ", &length) == 1) {
		// 	printf("Length = %d\n", length);
		// }
	}

	/* Read and display the HTTP Body */
	n = Rio_readlineb(&rio, buf, MAXBUF);
	while (n > 0) {
		// printf("%s", buf);
		n = Rio_readlineb(&rio, buf, MAXBUF);
	}
}

void *stress_test()
{
	int clientfd, i;
	for (i = 0; i < 200; i++)
	{
		clientfd = Open_clientfd(host, port);
		clientSend(clientfd, filename);
		clientPrint(clientfd);
		Close(clientfd);
	}
	return 0;
}

int main(int argc, char *argv[])
{
	if (argc != 4) {
		fprintf(stderr, "Usage: %s <host> <port> <filename>\n", argv[0]);
		exit(1);
	}

	host = argv[1];
	port = atoi(argv[2]);
	filename = argv[3];
	int clientfd;
	// char *filenames[10];

	// filenames[0] = strdup("/1.cgi?3");
	// filenames[1] = strdup("/1_.cgi?3");
	// filenames[2] = strdup("/1__.cgi?3");
	// filenames[3] = strdup("/2.cgi?3");
	// filenames[4] = strdup("/3.cgi?3");
	// filenames[5] = strdup("/a.html");
	// filenames[6] = strdup("/ab.html");
	// filenames[7] = strdup("/abc.html");
	// filenames[8] = strdup("/abcd.html");

	clientfd = Open_clientfd(host, port);
	clientSend(clientfd, "/abcd.html");

	clientfd = Open_clientfd(host, port);
	clientSend(clientfd, "/");

	clientfd = Open_clientfd(host, port);
	clientSend(clientfd, "/a.html");
	
	// clientfd = Open_clientfd(host, port);
	// clientSend(clientfd, "/boo.html");

	// clientfd = Open_clientfd(host, port);
	// clientSend(clientfd, "/1_.html");

	// clientfd = Open_clientfd(host, port);
	// clientSend(clientfd, "/abc.html");

	// clientfd = Open_clientfd(host, port);
	// clientSend(clientfd, "/ab.html");

	// clientfd = Open_clientfd(host, port);
	// clientSend(clientfd, "/1__.html");

	// clientPrint(clientfd);
	// Close(clientfd);

	// Build a threadpool to siege our server.
	// int clients = 8;
	// pthread_t cid[clients];
	// int i;
	// for (i = 0; i < clients; i++)
	// {
	// 	pthread_create(&cid[i], NULL, stress_test, NULL);
	// }
	// for (i = 0; i < clients; i++)
	// {
	// 	pthread_join(cid[i], NULL);
	// }

	return 0;
}
