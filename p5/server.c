#include <stdio.h>
#include "udp.h"
#include "mfs.h"
#include <sys/types.h>
#include <unistd.h>

char* filename;
int* port;
int fd = 0;

/**
 * portnum: the port number that the file server should listen on.
 * file-system-image: a file that contains the file system image.
 * If the file system image does not exist, you should create it and properly 
 * initialize it to include an empty root directory.
 */
void getargs(int *port, int argc, char *argv[]){
	if (argc != 3){
		// prompt> server [portnum] [file-system-image]
		fprintf(stderr, "Usage: %s [portnum] [file-system-image]\n", argv[0]);
		exit(1);
	}
	*port = atoi(argv[1]);
	filename = strdup(argv[2]);
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
		if (pinum < 0 || pinum >= MFS_BLOCK_SIZE) {
			return -1;
		}

		int imapIdx = pinum/16;
		lseek(fd, 0, SEEK_SET);

		int* checkPointVal;
		read(fd, (void*)checkPointVal, 4);

		// Setting it to (end - 16384) bytes
		// 16384 bytes as 256*64
		// 256 is number of IMap entries
		// 64 is the size of each I map
		lseek(fd, (*checkPointVal) + imapIdx -16384, SEEK_SET);
		
		int iNodeMapIdx = pinum%16;
		// Each imapIdx is of 4 bytes, so multiplying by 4
		lseek(fd, (iNodeMapIdx*4), SEEK_CUR);
		
		int* location;
		
		if (read(fd, (void*)location, 4) < 0) {
			return -1;
		} 

		if (*location > *checkPointVal) {
			return -1;
		}

		lseek(fd, 0, SEEK_SET);
		lseek(fd, (*location), SEEK_SET);
		
		MFS_Stat_t* dirIMap;
		if (read(fd, (void*)dirIMap, sizeof(MFS_Stat_t)) < 0) {
			return -1;
		}  

		if (dirIMap->type != MFS_DIRECTORY) {
			return -1;
		}
		
		int* iNodePtrs[14];
		int i = 0;
		if (read(fd, (void*)iNodePtrs, 14*sizeof(int*)) < 0) {
			return -1;
		}

		MFS_DirEnt_t* dirEntry;
		for (; i < 14; ++i) {
			lseek(fd, *(iNodePtrs[i]), SEEK_SET);

			if (read(fd, (void*)dirEntry, sizeof(MFS_DirEnt_t)) < 0) {
				return -1;
			}

			if (dirEntry->inum == -1) {
				continue;
			}
			
			if (strcmp(dirEntry->name, name) == 0) {
				return dirEntry->inum;
			}
		}
		return -1;
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

	fd = open(name, O_RDWR | O_TRUNC | O_CREAT, S_IRWXU);

	if (fd < 0) {
		char error_message[30] = "An error has occurred\n";
		write(STDERR_FILENO, error_message, strlen(error_message));
		exit(1);
	}

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
	
	getargs(&port, argc, argv);
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
	// start_server(argc, argv);

	// To manage image on disk use: open(), read(), write(), lseek(), close(), fsync()
	// Note: Unused entries in the inode map and unused direct pointers in the inodes should 
	// have the value 0. This condition is required for the mfscat and mfsput tools to work correctly.


	return 0;
}
