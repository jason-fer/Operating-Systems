#include <stdio.h>
#include "udp.h"
#include "mfs.h"

int 
srv_Init(char *hostname, int port){
  // @todo: write this method
  return 0;
}

/**
 * Takes the parent inode number (which should be the inode number of a 
 * directory) and looks up the entry name in it. The inode number of name is 
 * returned. Success: return inode number of name; failure: return -1. Failure 
 * modes: invalid pinum, name does not exist in pinum.
 */
int 
srv_Lookup(int pinum, char *name){
  // @todo: write this method
  return 0;
}

/**
 * Returns some information about the file specified by inum. Upon success, 
 * return 0, otherwise -1. The exact info returned is defined by MFS_Stat_t. 
 * Failure modes: inum does not exist.
 */
int 
srv_Stat(int inum, MFS_Stat_t *m){
  // @todo: write this method
  return 0;
}

/**
 * Writes a block of size 4096 bytes at the block offset specified by block. 
 * Returns 0 on success, -1 on failure. Failure modes: invalid inum, invalid 
 * block, not a regular file (because you can't write to directories).
 */
int 
srv_Write(int inum, char *buffer, int block){
  // @todo: write this method
  return 0;
} 

/**
 * Reads a block specified by block into the buffer from file specified by inum.
 * The routine should work for either a file or directory; directories should 
 * return data in the format specified by MFS_DirEnt_t. Success: 0, failure: -1.
 * Failure modes: invalid inum, invalid block.
 */
int 
srv_Read(int inum, char *buffer, int block){
  // @todo: write this method
  return 0;
}

/**
 * Makes a file (type == MFS_REGULAR_FILE) or directory (type == MFS_DIRECTORY) 
 * in the parent directory specified by pinum of name name . Returns 0 on 
 * success, -1 on failure. Failure modes: pinum does not exist, or name is too 
 * long. If name already exists, return success (think about why).
 */
int 
srv_Creat(int pinum, int type, char *name){
  // @todo: write this method
  return 0;
}

/**
 * Removes the file or directory name from the directory specified by pinum. 
 * 0 on success, -1 on failure. 
 * Failure modes: pinum does not exist, directory is NOT empty. Note that the 
 * name not existing is NOT a failure by our definition (think about why this 
 * might be).
 */
int 
srv_Unlink(int pinum, char *name){
  // @todo: write this method
  return 0;
}

/**
 * Just tells the server to force all of its data structures to disk and 
 * shutdown by calling exit(0). This interface will mostly be used for testing 
 * purposes.
 */
int 
srv_Shutdown(){
  // @todo: write this method
  return 0;
}

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