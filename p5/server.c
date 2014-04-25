#include <stdio.h>
#include "udp.h"
#include "mfs.h"

#define BUFFER_SIZE (4096)
// The server will need to decode the struct we send over & interpret the 
// following commands:
// int MFS_Init(char *hostname, int port);
// int MFS_Lookup(int pinum, char *name);
// int MFS_Stat(int inum, MFS_Stat_t *m);
// int MFS_Write(int inum, char *buffer, int block);
// int MFS_Read(int inum, char *buffer, int block);
// int MFS_Creat(int pinum, int type, char *name);
// int MFS_Unlink(int pinum, char *name);
// int MFS_Shutdown();

/**
 * portnum: the port number that the file server should listen on.
 * file-system-image: a file that contains the file system image.
 * If the file system image does not exist, you should create it and properly 
 * initialize it to include an empty root directory.
 */
void getargs(int *port, char **filename, int argc, char *argv[])
{
	if (argc != 3){
		// prompt> server [portnum] [file-system-image]
		fprintf(stderr, "Usage: %s [portnum] [file-system-image]\n", argv[0]);
		exit(1);
	}
	*port = atoi(argv[1]);
	*filename = strdup(argv[2]);
	// printf("port: %d\n", *port);
	// printf("file image name: %s\n", *filename);
}

/**
 * E.g. usage: ./server 10000 tempfile
 */
int
main(int argc, char *argv[])
{
	int sd, port;
	char *filename; //@todo: implement filename
	
	getargs(&port, &filename, argc, argv);

	sd = UDP_Open(port);
	assert(sd > -1);

	printf("SERVER:: waiting in loop\n");

	while (1) {
		struct sockaddr_in s;
		char buffer[BUFFER_SIZE];
		int rc = UDP_Read(sd, &s, buffer, BUFFER_SIZE);
		if (rc > 0) {
			printf("SERVER:: read %d bytes (message: '%s')\n", rc, buffer);
			char reply[BUFFER_SIZE];
			sprintf(reply, "reply");
			rc = UDP_Write(sd, &s, reply, BUFFER_SIZE);
			}
	}

	return 0;
}

// To manage image on disk use: open(), read(), write(), lseek(), close(), fsync()