#include <stdio.h>
#include "udp.h"
#include "mfs.h"

/**
 * portnum: the port number that the file server should listen on.
 * file-system-image: a file that contains the file system image.
 * If the file system image does not exist, you should create it and properly 
 * initialize it to include an empty root directory.
 */
void getargs(int *port, char **filename, int argc, char *argv[]){
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

int srv_Init(){
	// Does this method really need to do anything? I'm thinking no....
	printf("SERVER:: you called MFS_Init\n");
	return 0;
}

/**
 * Takes the parent inode number (which should be the inode number of a 
 * directory) and looks up the entry name in it. The inode number of name is 
 * returned. Success: return inode number of name; failure: return -1. Failure 
 * modes: invalid pinum, name does not exist in pinum.
 */
int srv_Lookup(int pinum, char *name){
	// @todo: write this method
	printf("SERVER:: you called MFS_Lookup\n");
	return 0;
}

/**
 * Returns some information about the file specified by inum. Upon success, 
 * return 0, otherwise -1. The exact info returned is defined by MFS_Stat_t. 
 * Failure modes: inum does not exist.
 */
int srv_Stat(int inum, MFS_Stat_t *m){
	// @todo: write this method
	printf("SERVER:: you called MFS_Stat\n");
	return 0;
}

/**
 * Writes a block of size 4096 bytes at the block offset specified by block. 
 * Returns 0 on success, -1 on failure. Failure modes: invalid inum, invalid 
 * block, not a regular file (because you can't write to directories).
 */
int srv_Write(int inum, char *buffer, int block){
	// @todo: write this method
	printf("SERVER:: you called MFS_Write\n");
	return 0;
} 

/**
 * Reads a block specified by block into the buffer from file specified by inum.
 * The routine should work for either a file or directory; directories should 
 * return data in the format specified by MFS_DirEnt_t. Success: 0, failure: -1.
 * Failure modes: invalid inum, invalid block.
 */
int srv_Read(int inum, char *buffer, int block){
	// @todo: write this method
	printf("SERVER:: you called MFS_Read\n");
	return 0;
}

/**
 * Makes a file (type == MFS_REGULAR_FILE) or directory (type == MFS_DIRECTORY) 
 * in the parent directory specified by pinum of name name . Returns 0 on 
 * success, -1 on failure. Failure modes: pinum does not exist, or name is too 
 * long. If name already exists, return success (think about why).
 */
int srv_Creat(int pinum, int type, char *name){
	// @todo: write this method
	printf("SERVER:: you called MFS_Creat\n");
	return 0;
}

/**
 * Removes the file or directory name from the directory specified by pinum. 
 * 0 on success, -1 on failure. 
 * Failure modes: pinum does not exist, directory is NOT empty. Note that the 
 * name not existing is NOT a failure by our definition (think about why this 
 * might be).
 */
int srv_Unlink(int pinum, char *name){
	// @todo: write this method
	printf("SERVER:: you called MFS_Unlink\n");
	return 0;
}

/**
 * Just tells the server to force all of its data structures to disk and 
 * shutdown by calling exit(0). This interface will mostly be used for testing 
 * purposes.
 */
int srv_Shutdown(int rc, int sd, struct sockaddr_in s, struct msg_r m){
	// Notify client we are shutting down; this is the only method completely 
	// tied to a client call.
	rc = UDP_Write(sd, &s, (char *) &m, sizeof(struct msg_r));
	// @todo: we probably need to call fsync (or an equivalent) before exit!
	printf("SERVER:: you called MFS_Shutdown\n");
	exit(0);
}

int call_rpc_func(int rc, int sd, struct sockaddr_in s, struct msg_r m){

	switch(m.method){
		case M_Init:
		 sprintf(m.reply, "MFS_Init");
		 srv_Init();
		 break;
		case M_Lookup:
		 sprintf(m.reply, "MFS_Lookup");
		 srv_Lookup(m.pinum, m.name);
		 break;
		case M_Stat:
		 sprintf(m.reply, "MFS_Stat");
		 srv_Stat(m.inum, &m.mfs_stat);
		 break;
		case M_Write:
		 sprintf(m.reply, "MFS_Write");
		 srv_Write(m.inum, m.buffer, m.block);
		 break;
		case M_Read:
		 sprintf(m.reply, "MFS_Read");
		 srv_Read(m.inum, m.buffer, m.block);
		 break;
		case M_Creat:
		 sprintf(m.reply, "MFS_Creat");
		 srv_Creat(m.pinum, m.type, m.name);
		 break;
		case M_Unlink:
		 sprintf(m.reply, "MFS_Unlink");
		 srv_Unlink(m.pinum, m.name);
		 break;
		case M_Shutdown:
		 sprintf(m.reply, "MFS_Shutdown");
		 srv_Shutdown(rc, sd, s, m);
		 break;
		default:
		 printf("SERVER:: method does not exist\n");
		 sprintf(m.reply, "Failed");
		 break;
	}

	// printf("reply: %s\n", m.reply);
	return UDP_Write(sd, &s, (char *) &m, sizeof(struct msg_r));
}

void start_server(int argc, char *argv[]){
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
			rc = call_rpc_func(rc, sd, s, m);
			// printf("SERVER:: message %d bytes (message: '%s')\n", rc, m.buffer);
		}
	}
}

/**
 * E.g. usage: ./server 10000 tempfile
 */
int main(int argc, char *argv[]){

	// Disable this to test methods without running the server...
	start_server(argc, argv);

	// To manage image on disk use: open(), read(), write(), lseek(), close(), fsync()
	// Note: Unused entries in the inode map and unused direct pointers in the inodes should 
	// have the value 0. This condition is required for the mfscat and mfsput tools to work correctly.


	return 0;
}