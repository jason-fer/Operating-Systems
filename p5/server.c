#include <stdio.h>
#include "udp.h"
#include "mfs.h"
#include <sys/types.h>
#include <unistd.h>

char* filename = "example.img";
int* port;
int fd = 0;
int inumMax = 0;

/**
 * portnum: the port number that the file server should listen on.
 * file-system-image: a file that contains the file system image.
 * If the file system image does not exist, you should create it and properly 
 * initialize it to include an empty root directory.
 */
void getargs(int *port, int argc, char *argv[]){
	if (argc != 3){
		fprintf(stderr, "Usage: %s [portnum] [file-system-image]\n", argv[0]);
		exit(1);
	}
	*port = atoi(argv[1]);
	filename = strdup(argv[2]);
	// printf("port: %d\n", *port);
	// printf("file image name: %s\n", *filename);
}

/**
 * Use the checkpoint region & imap piece to find & return the inode location
 */
int get_inode_location(int inum) {
	// Set pointer at the front of the file
	lseek(fd, 0, SEEK_SET);

	// Read result into the address of the checkPointVal
	int checkPointVal = 0;
	int rc = read(fd, &checkPointVal, sizeof(int));
	if (rc < 0) {
		return -1;
	}

	// There are 256 imaps, each with 16 inodes references (4096 total inodes)
	int imapPieceIdx = inum / 16;
	// +4 because the first checkpoint region entry points to the end of the log
	// *4 because each imap ref is 4 a byte int
	lseek(fd, (imapPieceIdx * 4) + 4, SEEK_SET);

	// Now we read from the checkpoint region to find where the imap piece is
	int locationToPiece = 0;
	if (read(fd, &locationToPiece, 4) < 0) {
		return -1;
	}

	// We found our imap piece; now use modulus to find one of 16 inode refs
	int iNodeMapIdx = inum % 16;
	// Each imapIdx is of 4 bytes, so multiplying by 4
	lseek(fd, locationToPiece + (iNodeMapIdx * 4), SEEK_SET);
	
	// printf ("rc value %d\n",rc);
	// printf ("size val %lu\n",sizeof(int));
	// printf ("checkPointVal %d\n", checkPointVal);
	// printf ("locationToPiece %d\n", locationToPiece);
	
	// Now read from the imap piece to find the inode ref
	int location = 0;
	rc = read(fd, &location, sizeof(int));
	if (rc < 0) {
		return -1;
	}
	if (location > checkPointVal) {
		return -1;
	}
	return location;
}

int srv_Init(){
	printf("SERVER:: you called MFS_Init\n");
	fd = open(filename, O_RDWR | O_CREAT, S_IRWXU);
	if (fd < 0) {
		char error_message[30] = "An error has occurred\n";
		write(STDERR_FILENO, error_message, strlen(error_message));
		exit(1);
	}
	return 0;
}

/**
 * Takes the parent inode number (which should be the inode number of a 
 * directory) and looks up the entry name in it. The inode number of name is 
 * returned. Success: return inode number of name; failure: return -1. Failure 
 * modes: invalid pinum, name does not exist in pinum.
 */
int srv_Lookup(int pinum, char *name) {
	printf("SERVER:: you called MFS_Lookup\n");
	// Ignore invalid parent inode numbers
	if (pinum < 0 || pinum >= MFS_BLOCK_SIZE) {
		return -1;
	}

	int location = get_inode_location(pinum);
	if (location < 0) {
		return -1;
	}
	// printf ("rc value %d\n",rc);
	// printf ("location %d\n", location);

	// Read in the inode directory map structure
	lseek(fd, location, SEEK_SET);
	Inode_t* dirIMap = malloc(sizeof(Inode_t));
	if (read(fd, dirIMap, sizeof(Inode_t)) < 0) {
		free(dirIMap);
		dirIMap = NULL;
		return -1;
	}

	// Parent inode numbers can only belong to directories
	if (dirIMap->type != MFS_DIRECTORY) {
		free(dirIMap);
		dirIMap = NULL;
		return -1;
	}

	free(dirIMap);
	dirIMap = NULL;

	int iNodePtr = -1;
	int i = 0;
	MFS_DirEnt_t dirEntry;

	// Read dir inode structure: 14x pointers to 64 byte directory blocks
	for (; i < 14; ++i) {
		lseek(fd, (location + sizeof(Inode_t) + i * sizeof(int)), SEEK_SET);
		iNodePtr = -1;
		if (read(fd, &iNodePtr, sizeof(int)) < 0) {
			return -1;
		}
		if (iNodePtr == 0) {
			continue; 
		}
		// printf ("iNode Ptr = %d\n",iNodePtr);
		lseek(fd, (iNodePtr), SEEK_SET);
 
		int count = 0;
		while(count != 64) {
			++count;
			// Reading a directory entry moves the file pointer forward to the next
			if (read(fd, &dirEntry, sizeof(MFS_DirEnt_t)) < 0) {
				continue;
			}
			// Ignore invalid inode numbers
			if (dirEntry.inum == -1) {
				continue;
			}
			// printf ("dirEntry.inum %d\n", dirEntry.inum);
			// printf ("dirEntry.name %s\n", dirEntry.name);
			if (strcmp(dirEntry.name, name) == 0) {
				return dirEntry.inum;
			}
		}
	}

	return -1;
}

/**
 * Reads a block specified by block into the buffer from file specified by inum.
 * The routine should work for either a file or directory; directories should 
 * return data in the format specified by MFS_DirEnt_t. Success: 0, failure: -1.
 * Failure modes: invalid inum, invalid block.
 */
int srv_Read(int inum, char *buffer, int block) {

  printf("SERVER:: you called MFS_Read\n");

  int location = get_inode_location(inum);

  Inode_t* currInode = malloc(sizeof(Inode_t));

  lseek(fd, location, SEEK_SET);

  if (read(fd, currInode, sizeof(Inode_t)) < 0) {
    free(currInode);
    currInode = NULL;
    return -1;
  }
  
  lseek(fd, location+sizeof(Inode_t)+(block)*sizeof(int), SEEK_SET); // setting it to block

  int iNodePtr = -1;

  if ( read(fd, &iNodePtr, sizeof(int)) < 0) {
    free(currInode);
    currInode = NULL;
    return -1;
  }

  if (iNodePtr == 0) {
    return -1;
  }

  lseek(fd, iNodePtr, SEEK_SET);

  if (currInode->type == MFS_REGULAR_FILE) {
    int size = -1;
    if (block > (currInode->size - 1)/MFS_BLOCK_SIZE) {
      size = MFS_BLOCK_SIZE;
    } else if (block == (currInode->size - 1)/MFS_BLOCK_SIZE) {
      size = (currInode->size)%MFS_BLOCK_SIZE ;
    }
      
    free(currInode);
    currInode = NULL;
      
    if (size < 0) {
      return -1;
    }

    /* buffer = malloc(size*sizeof(char)); */
    if (read(fd, buffer, size) < 0) {
      return -1;
    }
      
    return 0;
  } else {
    int count         = 0;
    int actualEntries = 0;
	MFS_DirEnt_t dirEntry;

    while(count != 64) {
      ++count;
      if (read(fd, &dirEntry, sizeof(MFS_DirEnt_t)) < 0) {
        continue;
      }
        
      if (dirEntry.inum == -1) {
        continue;
      }
      ++actualEntries;
    }
      
    MFS_DirEnt_t* returnVal = malloc(actualEntries*sizeof(actualEntries));
      
    count          = 0;
    int numEntries = 0;

    while(count != 64) {
      ++count;

      if (numEntries == actualEntries) {
        break;
      }

      if (read(fd, &dirEntry, sizeof(MFS_DirEnt_t)) < 0) {
        continue;
      }
        
      if (dirEntry.inum == -1) {
        continue;
      }
        
      returnVal[numEntries++] = dirEntry;
    }

    buffer = (char*)returnVal;
    free(returnVal);
    return 0;
  }
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
	// Jason
	return 0;
} 

int set_next_inum(int pinum, int type, char *name){
	// Set pointer at the front of the file
	lseek(fd, 0, SEEK_SET);
	printf("Server:: finding max inum\n");

	// Read result into the address of the checkPointVal
	int ckpt_rg = 0;
	int rc;
	int i = 0;
	int j = 0;
	int count = 0; //, prev = 0;
	for (; i <= 256; i++){
		// Make sure we read from the right position...
		lseek(fd, i * 4, SEEK_SET);
		rc = read(fd, &ckpt_rg, sizeof(int));
		if(i == 0){
			printf("end of log address: %d\n", ckpt_rg);
			continue;
		}
		// prev = ckpt_rg;
		count++;
		// printf("ckpt_rg %d: %d, ckpt_rg - prev:%d\n", i, ckpt_rg, ckpt_rg - prev);
		if(count > 4) break;
		// Find the imap piece
		lseek(fd, ckpt_rg, SEEK_SET);
		// Now we read from the checkpoint region to find where the imap piece is
		int inode_loc = 0;
		// Dump the contents of this imap:
		for(j = 0; j < 16; j++){
			if (read(fd, &inode_loc, 4) < 0) {
				printf("error!!!\n");
				exit(0);
			}
			int the_imap = i - 1;
			int the_inum = (j + ((i - 1) * 16));
			// printf("inode_loc: %d, ckpt_rg: %d, inum: %d, j: %d, i: %d \n", inode_loc, (ckpt_rg - 64) + (j * 4), the_inum, j, i);
			printf("inode_loc: %d, ckpt_rg: %d, inum: %d, imap#: %d \n", inode_loc, (ckpt_rg - 64) + (j * 4), the_inum, the_imap);
			if(inode_loc == 0){
				// This becomes our next inode; we need to allocate memory here... 128 bytes?
				// 64 bytes for the inode 
				// 64 bytes for the disk or directory
				// update the checkpoint region
				// init all the entries in the inode & set the size to zero & set the type
			}
			
		}
	}

	return -1;
}

/**
 * Makes a file (type == MFS_REGULAR_FILE) or directory (type == MFS_DIRECTORY) 
 * in the parent directory specified by pinum of name *name. Returns 0 on 
 * success, -1 on failure. Failure modes: pinum does not exist, or name is too 
 * long. If name already exists, return success.
 */
int srv_Creat(int pinum, int type, char *name){
	printf("SERVER:: you called MFS_Creat\n");

	// 1-create entry in an existing area
	// 2-create entry to a new area
	int rs = srv_Lookup(pinum, name);
	if (strcmp((char*) &rs, name) == 0){
		// The directory already exists; we are done
		return 0;
	}

	int location = get_inode_location(pinum);
	if (location < 0) {
		return -1;
	}

	set_next_inum(pinum, type, name);
	return 0;
	exit(0);
	
	// if every imap is full, point the appropriate CR region entry to a new imap
	// if we created a new imap, initialze all the values to 0
	// save the imap location for this node; we will point it to our inode

	// Start--> this is almost indentical to srv_Lookup
	// Read in the inode directory map structure
	/*lseek(fd, location, SEEK_SET);
	Inode_t* dirIMap = malloc(sizeof(Inode_t));
	if (read(fd, dirIMap, sizeof(Inode_t)) < 0) {
		free(dirIMap);
		dirIMap = NULL;
		return -1;
	}

	// Parent inode numbers can only belong to directories
	if (dirIMap->type != MFS_DIRECTORY) {
		free(dirIMap);
		dirIMap = NULL;
		return -1;
	}

	free(dirIMap);
	dirIMap = NULL;

	int iNodePtr = -1;
	int i = 0;
	int dirIsFound = 0;
	MFS_DirEnt_t dirEntry;

	// Read dir inode structure: 14x pointers to 64 byte directory blocks
	for (; i < 14; ++i) {
		lseek(fd, (location + sizeof(Inode_t) + i * sizeof(int)), SEEK_SET);
		iNodePtr = -1;
		if (read(fd, &iNodePtr, sizeof(int)) < 0) {
			return -1;
		}
		if (iNodePtr == 0) {
			continue; 
		}
		// printf ("iNode Ptr = %d\n",iNodePtr);
		lseek(fd, (iNodePtr), SEEK_SET);
 
		int count = 0;
		while(count != 64) {
			++count;
			// Reading a directory entry moves the file pointer forward to the next
			if (read(fd, &dirEntry, sizeof(MFS_DirEnt_t)) < 0) {
				continue;
			}
			// Ignore invalid inode numbers
			if (dirEntry.inum == -1) {
				dirIsFound = 1;
				// Set the next free inum
				// point the next free imap location to this new inode
			}
		}
	}*/
	// End --> of the part that is almost identical to srv_Lookup

	// if(! dirIsFound){
	// 	printf("failed to find free directory space\n");
	// 	return -1;
	// }

	// if we created a file, we are done; set the size to 0
	// if we created directory: add "." and ".."; if this is the root, both . & ..
	// point to "0", otherwise . points to this directory and .. points to pinum

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
	// Jason
	return 0;
}

void fs_Shutdown(){
	int rs = fsync(fd);
	assert(rs != -1);
	rs = close(fd);
	assert(rs != -1);
	printf("SERVER:: you called MFS_Shutdown\n");
	exit(0);
}

/**
 * Just tells the server to force all of its data structures to disk and 
 * shutdown by calling exit(0). This interface will mostly be used for testing 
 * purposes.
 */
/*int srv_Shutdown(int rc, int sd, struct sockaddr_in s, struct msg_r m){
		// Notify client we are shutting down; this is the only method completely 
		// tied to a client call.
		rc = UDP_Write(sd, &s, (char *) &m, sizeof(struct msg_r)); 
		// @todo: we probably need to call fsync (or an equivalent) before exit!
		fs_Shutdown();
		// This will never happen....
		return 0;
}*/

/*int call_rpc_func(int rc, int sd, struct sockaddr_in s, struct msg_r m){

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

	printf("reply: %s\n", m.reply);
	return 0;
	return UDP_Write(sd, &s, (char *) &m, sizeof(struct msg_r)); 
}*/

/*void start_server(int argc, char *argv[]){
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
			printf("SERVER:: message %d bytes (message: '%s')\n", rc, m.buffer);
		}
	 } 
}*/

/**
 * E.g. usage: ./server 10000 tempfile
 */
int main(int argc, char *argv[]){

	// Disable this to test methods without running the server...
	// start_server(argc, argv);

	// To manage image on disk use: open(), read(), write(), lseek(), close(), fsync()
	// Note: Unused entries in the inode map and unused direct pointers in the inodes should 
	// have the value 0. This condition is required for the mfscat and mfsput tools to work correctly.
	// int inum;

	srv_Init();
	srv_Creat(0, MFS_DIRECTORY, "awesome-dir");
	// Can we find the base dir?
	// inum = srv_Lookup(0, "dir");
	// printf("/dir is inum: %d\n", inum); // we expect 1
	// inum = srv_Lookup(0, "code");
	// printf("/code is inum: %d\n", inum); // we expect 2
	// inum = srv_Lookup(0, "it's");
	// printf("/it's is inum: %d\n", inum); // we expect 6
	// inum = srv_Lookup(2, "helloworld.c");
	// printf("/helloworld is inum: %d\n", inum); // we expect 6
  // char* buffer = malloc(4096*sizeof(char));
  // int rc = srv_Read(3, buffer, 0);
  // if (rc == 0) {
  //   printf("From srv_Read\n");
  //   printf("%s\n", buffer);
  //   free(buffer);
  // }
	return 0;
}





/*// Get max inum if we don't have one already
if(inumMax == 0){
	inumMax = get_max_inum();
	// int loc = get_inode_location(inumMax);
	// printf("max_inum: %d, loc: %d\n", inumMax, loc);
}*/

/**
 * Alternative way to get max inum; save it for now.
 */
/*int get_max_inum(){
	int next_inum = 0;
	int loc = 1;
	int max = 0;
	while(loc > 0){ 
		loc = get_inode_location(next_inum);
		if(loc <= 0) break;
		if(next_inum > max) max = next_inum;
		max = max_dir_inum(loc, max);
		// printf("max inum: %d\n", max);
		next_inum++;
	}
	return max;
}*/

/*int max_dir_inum(int location, int max_inum){
	lseek(fd, location, SEEK_SET);
	Inode_t* dirIMap = malloc(sizeof(Inode_t));
	if (read(fd, dirIMap, sizeof(Inode_t)) < 0) {
		free(dirIMap);
		dirIMap = NULL;
		return max_inum;
	}

	// Parent inode numbers can only belong to directories
	if (dirIMap->type != MFS_DIRECTORY) {
		free(dirIMap);
		dirIMap = NULL;
		return max_inum;
	}

	free(dirIMap);
	dirIMap = NULL;

	int iNodePtr = -1;
	int i = 0;
	MFS_DirEnt_t dirEntry;

	// Read dir inode structure: 14x pointers to 64 byte directory blocks
	for (; i < 14; ++i) {
		lseek(fd, (location + sizeof(Inode_t) + i * sizeof(int)), SEEK_SET);
		iNodePtr = -1;
		if (read(fd, &iNodePtr, sizeof(int)) < 0) {
			return max_inum;
		}
		if (iNodePtr == 0) {
			continue; 
		}
		// printf ("iNode Ptr = %d\n",iNodePtr);
		lseek(fd, (iNodePtr), SEEK_SET);
 
		int count = 0;
		while(count != 64) {
			++count;
			// Reading a directory entry moves the file pointer forward to the next
			if (read(fd, &dirEntry, sizeof(MFS_DirEnt_t)) < 0) {
				continue;
			}
			// Ignore invalid inode numbers
			if (dirEntry.inum == -1) {
				continue;
			}
			// printf ("'%s': %d\n", dirEntry.name, dirEntry.inum);
			if(dirEntry.inum > max_inum) max_inum = dirEntry.inum;
		}
	}
	// printf("max inum: %d\n", max_inum);
	return max_inum;
}*/