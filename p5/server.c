#include <stdio.h>
#include "udp.h"
#include "mfs.h"

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
		struct msg_r m;
		int rc = UDP_Read(sd, &s, (char *) &m, sizeof(struct msg_r));
		if (rc > 0) {
			printf("SERVER:: message %d bytes (message: '%s')\n", rc, m.buffer);
			rc = UDP_Write(sd, &s, (char *) &m, sizeof(struct msg_r));
			}
	}

	return 0;
}

// To manage image on disk use: open(), read(), write(), lseek(), close(), fsync()

// Note: Unused entries in the inode map and unused direct pointers in the inodes should 
// have the value 0. This condition is required for the mfscat and mfsput tools to work correctly.