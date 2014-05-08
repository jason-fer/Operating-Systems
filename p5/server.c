#include <stdio.h>
#include "udp.h"
#include "mfs.h"
#include <sys/types.h>
#include <unistd.h>

char* filename = NULL;
// char* filename = "example.img";
// char* filename = "bare.img";
int* port;
int fd = 0;
int inumMax = 0;

// Dump contents of the current file
void dump_file_inode(int fd, int file_loc, int file_size){
	int inode_blk_ptrs[14];
	int i = 0;
	char buffer[MFS_BLOCK_SIZE];

	if(file_size <= 0){ 
		printf("file_loc:%d (empty file)\n", file_loc);
	}

	lseek(fd, (file_loc + sizeof(Inode_t)), SEEK_SET);
	if (read(fd, &inode_blk_ptrs, sizeof(int) * 14) < 0) { return; }

	// Read dir inode structure: 14x pointers to 64 byte directory blocks
	for (; i < 14; ++i) {
		if(inode_blk_ptrs[i] == 0) continue; // inode not in use!
		// printf("block ptr:%d, loc:%d\n", i, inode_blk_ptrs[i]);
		lseek(fd, inode_blk_ptrs[i], SEEK_SET);
 
		printf("inode_blk_ptrs[%i]:%d, data block contents: \n%s\n",i, inode_blk_ptrs[i], buffer);

		// XXXXXXXXXXXXXXXXXXXXXX TEMP XXXXXXXXXXXXXXXXXXXXXXXXXXXXX
		// prevents crazy log dump...
		if(file_size > MFS_BLOCK_SIZE){
			file_size = MFS_BLOCK_SIZE;
		}
		// XXXXXXXXXXXXXXXXXXXXXX TEMP XXXXXXXXXXXXXXXXXXXXXXXXXXXXX
		while(file_size > 0) {
			// printf("curr file size: %d\n", file_size);
			// Keep reading forward on file...
			if(file_size >= MFS_BLOCK_SIZE){
				if (read(fd, &buffer, MFS_BLOCK_SIZE) < 0) { continue; }
			} else {
				if (read(fd, &buffer, file_size) < 0) { continue; }
			}
			printf("inode_blk_ptrs[%i]:%d, data block contents: \n%s\n",i, inode_blk_ptrs[i], buffer);
			file_size -= MFS_BLOCK_SIZE;
		}
	}

	return;
}

// Dump contents of the current dir
void dump_dir_inode(int fd, int dir_loc){
	int inode_block_ptrs[14];
	int i = 0;
	MFS_DirEnt_t dirEntry;

	lseek(fd, (dir_loc + sizeof(Inode_t)), SEEK_SET);
	if (read(fd, &inode_block_ptrs, sizeof(int) * 14) < 0) { return; }

	// Read dir inode structure: 14x pointers to 64 byte directory blocks
	for (; i < 14; ++i) {
		if(inode_block_ptrs[i] == 0) continue; // inode not in use!
		printf("inode:%d, loc:%d\n", i, inode_block_ptrs[i]);
		lseek(fd, inode_block_ptrs[i], SEEK_SET);
 
		int count = 0;
		while(count != 64) {
			++count;
			// Reading a directory entry moves the file pointer forward to the next
			if (read(fd, &dirEntry, sizeof(MFS_DirEnt_t)) < 0) { continue; }
			// Ignore invalid inode numbers
			if (dirEntry.inum == -1) { continue; }
			printf("inum:%d, dir:%s\n", dirEntry.inum, dirEntry.name);
		}
	}

	return;
}

// Generate a log dump of the file structure; expects a file
int dump_log(){
	// Set pointer at the front of the file
	lseek(fd, 0, SEEK_SET);
	printf("Server:: ################# Log Dump #################\n");

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
			printf("1st checkpoint ptr: %d\n", ckpt_rg);
			continue;
		}
		// prev = ckpt_rg;
		count++;
		// printf("ckpt_rg %d: %d, ckpt_rg - prev:%d\n", i, ckpt_rg, ckpt_rg - prev);
		if(count > 4) break;
		// Find the imap piece
		// lseek(fd, ckpt_rg, SEEK_SET);
		// Now we read from the checkpoint region to find where the imap piece is
		int inode_loc = 0;
		// Dump the contents of this imap:
		int read_loc = ckpt_rg;
		for(j = 0; j < 16; j++){ // Dump the imap structure
			// Force read in the right position...
			lseek(fd, read_loc, SEEK_SET);
			read_loc += 4; // increment read location..
			if (read(fd, &inode_loc, 4) < 0) {
				printf("error!!!\n");
				exit(0);
			}
			int the_imap = i - 1;
			int the_inum = (j + ((i - 1) * 16));

			if(inode_loc == 0) continue;
			// printf("inode_loc: %d, ckpt_rg: %d, inum: %d, j: %d, i: %d \n", inode_loc, (ckpt_rg - 64) + (j * 4), the_inum, j, i);
			printf("inode_loc: %d, ckpt_rg: %d, inum: %d, imap piece#: %d \n", inode_loc, (ckpt_rg - 64) + (j * 4), the_inum, the_imap);

			lseek(fd, inode_loc, SEEK_SET);
			Inode_t* curr_inode = malloc(sizeof(Inode_t));

			if (read(fd, curr_inode, sizeof(Inode_t)) < 0) { free(curr_inode); continue; }

			// Parent inode numbers can only belong to directories
			if (curr_inode->type != MFS_DIRECTORY) { // It's a file
				// printf("inode_loc:%d, file size:%d\n", inode_loc, curr_inode->size);
				dump_file_inode(fd, inode_loc, curr_inode->size);
			} else { // It's a directory
				dump_dir_inode(fd, inode_loc);
			}

			free(curr_inode);

			if(inode_loc == 0){
				// printf("our next inode goes here!!!\n"); return 0;
				// This becomes our next inode; we need to allocate memory here... 128 bytes?
				// 64 bytes for the inode 
				// 64 bytes for the disk or directory
				// next imap piece if applicable
				// update the checkpoint region
				// init all the entries in the inode & set the size to zero & set the type
			}
			
		}
	}

	return -1;
}

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
	printf("port: %d\n", *port);
	printf("file-image name: %s\n", filename);
}

/**
 * Use the checkpoint region & imap piece to find & return the inode location
 */
int get_inode_location(int inum) {

	// Ignore invalid inode numbers
	if (inum < 0 || inum >= MFS_BLOCK_SIZE) {
		return -1;
	}

	// printf ("Got inum as %d\n", inum);
	// printf ("Got filename as %s\n", filename);
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

int
initDisk() {
	fd = open(filename, O_RDWR | O_CREAT, S_IRWXU);
	if (fd < 0) {
		printf("Open error\n");
		exit(1);
	}

	lseek(fd, 0, SEEK_SET);
	int eol = (4 + 256*4 + 4096 + sizeof(Inode_t) + 14*sizeof(int) + 16*sizeof(int));
	if (write(fd, &eol, sizeof(int)) < 0) {
		return -1;
	}
	
	int imapPieceIdxArr[256];
	imapPieceIdxArr[0] = (4 + 256*4 + 4096 + sizeof(Inode_t) + 14*sizeof(int));

	int j = 1;
	for (; j < 256; ++j) {
		imapPieceIdxArr[j] = 0;
	}

	if (write(fd, &imapPieceIdxArr, 256*sizeof(int)) < 0) {
		return -1;
	}


	lseek(fd, 257*sizeof(int),SEEK_SET);

	MFS_DirEnt_t dirEntries[64];
	
	dirEntries[0].name[0] = '.';
	dirEntries[0].name[1] = '\0';
	dirEntries[0].inum    = 0;

	dirEntries[1].name[0] = '.';
	dirEntries[1].name[1] = '.';
	dirEntries[1].name[2] = '\0';
	dirEntries[1].inum    = 0;

	int i = 2;
	for (; i < 64; i++) {
		dirEntries[i].name[0] = '\0';
		dirEntries[i].inum    = -1;
	}

	if (write(fd, &dirEntries, 64*sizeof(MFS_DirEnt_t)) < 0) {
		return -1;
	}

	Inode_t curr_inode;
	curr_inode.size = 4096;
	curr_inode.type = MFS_DIRECTORY;

	if (write(fd, &curr_inode, sizeof(Inode_t)) < 0) {
		return -1;
	}
	
	int inode_ptrs[14];
	
	inode_ptrs[0] = 257*4;
	inode_ptrs[1] = 257*4;
	
	i = 2;
	for (; i < 14; ++i) {
	  inode_ptrs[i] = 0;
	}

	if (write(fd, &inode_ptrs, sizeof(int)*14) < 0) {
	  return -1;
	}

	int imapPiece = (4 + 256*4 + 4096);
	if (write(fd, &imapPiece, sizeof(int)) < 0) {
		return -1;
	}
	
	i = 1;
	int zero = 0;
	for (; i < 16; ++i) {
		if (write(fd, &zero, sizeof(int)) < 0) {
			return -1;
		}
	}

	fsync(fd);
	return 0;
}
 
int srv_Init() {
	printf("SERVER:: you called MFS_Init\n");
	fd = open(filename, O_RDWR, S_IRWXU);
	if (fd < 0) {
		return initDisk();
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
	// don't allow non-sensical blocks (lt 0, or gt 13)
	if(block < 0 || block > 13) return -1;

	printf("SERVER:: you called MFS_Read\n");

	int location = get_inode_location(inum);
	if (location < 0) {
	  return -1;
	}

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
	printf("SERVER:: you called MFS_Stat\n");
	int location = get_inode_location(inum);
	if (location < 0) {
	  return -1;
	}

	Inode_t* currInode = malloc(sizeof(Inode_t));

	lseek(fd, location, SEEK_SET);

	if (read(fd, currInode, sizeof(Inode_t)) < 0) {
		free(currInode);
		currInode = NULL;
		return -1;
	}
	
	m->size = currInode->size;
	m->type = currInode->type;
	free(currInode);
	currInode = NULL;
	return 0;
}

/**
 * Writes a block of size 4096 bytes at the block offset specified by block. 
 * Returns 0 on success, -1 on failure. Failure modes: invalid inum, invalid 
 * block, not a regular file (because you can't write to directories).
 */
int srv_Write(int inum, char *buffer, int block){
	// printf("*****************************buffer: !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n%s\n", buffer);
	printf("SERVER:: you called MFS_Write\n");
	// Check for valid block: 0 to 13 or invalid inum
	if(block < 0 || block > 13 || inum < 0 || inum >= MFS_BLOCK_SIZE) {
	  return -1;
	}

	// read the checkpoint region
	int eol_ptr = 0; // the end of LFS pointer; should always point to an imap
	lseek(fd, 0, SEEK_SET);
	if (read(fd, &eol_ptr, sizeof(int)) < 0) { 
	  return -1; 
	}
	// printf("!!!!!!!!!!!!!!!!!!!!!! orig EOL: %d\n", eol_ptr);

	// Read in our 256 imap pointers... each with 16 inode refs; 4096 max inums
	int ckpr_ptrs[256];
	if (read(fd, &ckpr_ptrs, sizeof(int) * 256) < 0) { 
	  return -1; 
	}

	int imap_index = inum / 16;
	// printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> imap index: %d\n", imap_index);
	int imap_loc = ckpr_ptrs[imap_index];

	// We found our imap piece; now use modulus to find one of 16 inode refs
	int inode_index = inum % 16;
	// Each imapIdx is of 4 bytes; multiply by 4
	int imap_ptrs[16];
	lseek(fd, imap_loc, SEEK_SET);
	if (read(fd, &imap_ptrs, sizeof(int) * 16) < 0) { 
	  return -1; 
	}

	// Now read from the imap piece to find the inode_loc
	int inode_loc = imap_ptrs[inode_index];
	// Confirm the inode_loc is valid
	if (inode_loc <= 0 || inode_loc > eol_ptr) { 
	  return -1; 
	}

	// Read inode to determine size / type
	Inode_t* curr_inode = malloc(sizeof(Inode_t));
	lseek(fd, inode_loc, SEEK_SET);

	if (read(fd, curr_inode, sizeof(Inode_t)) < 0) {
	  free(curr_inode); 
	  return -1; 
	}
	
	if (curr_inode->type == MFS_DIRECTORY) { 
	  free(curr_inode); 
	  return -1; 
	}
	
	// printf("file sizexxxxxxxxxxx: %d\n", curr_inode->size);
	
	// Determine the new file size
	int inode_ptrs[14];
	lseek(fd, (inode_loc + sizeof(Inode_t)), SEEK_SET);
	
	if (read(fd, &inode_ptrs, sizeof(int) * 14) < 0) { 
	  free(curr_inode); 
	  return -1; 
	}

	// printf("previous file size:%d\n", curr_inode->size);

	// Determine the file size

	int data_size = strlen(buffer);
	// Add the new buffer to the total file size
	int file_size = data_size;
	char tmp_buffer[MFS_BLOCK_SIZE];
	int i = 0;
	for (; i < 14; i++){
		// This is the block we are replacing; ignore it.
		if (i == block || inode_ptrs[i] <= 0) 
		  continue;

		lseek(fd, inode_ptrs[i], SEEK_SET);

		if (read(fd, &tmp_buffer, MFS_BLOCK_SIZE) < 0 ) { 
		  free(curr_inode); 
		  return -1; 
		}

		file_size += strlen(tmp_buffer);
	}

	curr_inode->size = file_size;
	// printf("new file size:%d\n", curr_inode->size);

	assert(curr_inode->size < MFS_BLOCK_SIZE);
	// Shouldn't this be 14*MFS_BLOCK_SIZE????

	assert(curr_inode->size >= 0);
	// printf("new size: %d\n", curr_inode->size);
	// printf("EOL:::::::::::::::::::::::::::%d\n", eol_ptr);

	// Set the pointer past the current 64 byte imap piece it points to:
	// (in Brandon's implementation the inode is last)
	// 1-append the buffer to a new data block (should we be writing MFS_BLOCK_SIZE?)
	lseek(fd, eol_ptr, SEEK_SET); // <--data goes to end of log...

	if(write(fd, buffer, MFS_BLOCK_SIZE) < 0) { 
	  free(curr_inode); 
	  return -1; 
	}

	// lseek(fd, eol_ptr + MFS_BLOCK_SIZE, SEEK_SET);
	// Record the new position of our data block
	inode_ptrs[block] = eol_ptr;
	// printf("WRITING FILE AT:::::::::::::::::::::::::::::::::%d\n", inode_ptrs[block]);
	// printf("WRITING FILE AT:::::::::::::::::::::::::::::::::%d\n", eol_ptr);

	// Set the eol_ptr just past the data block we just wrote (to the inode)
	eol_ptr += MFS_BLOCK_SIZE; // <--this is now our inode location...

	// 2-append the updated inode which points to the new data block
	if (write(fd, &curr_inode, sizeof(Inode_t)) < 0) { 
	  free(curr_inode); 
	  return -1; 
	}
	
	if(write(fd, &inode_ptrs, (sizeof(int) * 14)) < 0) { 
	  free(curr_inode); 
	  return -1; 
	}

	// Point the imap to our new inode
	imap_ptrs[inode_index] = eol_ptr;
	eol_ptr += (sizeof(Inode_t) + (sizeof(int) * 14)); // Size of inode + ptrs

	// 3-append the imap which now points to the new inode
	imap_loc = eol_ptr; // <--need to point checkpoint region to imap piece
	if(write(fd, &imap_ptrs, sizeof(int) * 16) < 0) { 
	  free(curr_inode); 
	  return -1; 
	}
	
	// Update our imap location
	eol_ptr += (sizeof(int) * 16); // Size of imap

	// 4-updated the checkpoint region which now points to the new imap
	lseek(fd, (imap_index * 4) + 4, SEEK_SET);
	if(write(fd, &imap_loc, sizeof(int)) < 0) { 
	  free(curr_inode); 
	  return -1; 
	}

	// printf("imap_loc XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX: %d\n", imap_loc);
	// lseek(fd, (imap_index * 4) + 4, SEEK_SET);
	// int temp;
	// read(fd, &temp, 4);
	// printf("TEMP XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX: %d\n", temp);
	// int tmp_val = (imap_index * 4);
	// printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> imap index: %d\n", tmp_val);

	// 5-update our end of log ptr to point to the new imap
	lseek(fd, 0, SEEK_SET);
	if(write(fd, &eol_ptr, sizeof(int)) < 0) { 
	  free(curr_inode);
	  return -1; 
	}

	fsync(fd);
	free(curr_inode);
	// exit(0);
	return 0;
} 


int
getNextInodeNumber() {
  int i = 0;
  for (; i < 4096; ++i) {
	if (get_inode_location(i) < 0) {
	  return i;
	}
  }
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
	if (rs == 0) {
		return 0;
	}

	int location = get_inode_location(pinum);
	if (location < 0) {
		return -1;
	}

	int inum = getNextInodeNumber();
	if (type == MFS_REGULAR_FILE) {
	  return mkFile(pinum, name, inum, location);
	} else if (type == MFS_DIRECTORY) {
	  return mkDir(pinum, name, inum, location);
	} 
	
	return -1;


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
	// directory block is based on #blocks in use * 4096

		/*
MFS_Creat(int pinum, int type, char *name)

The operation should create a new file with the given name in the
directory with inode number pinum. The type is either a regular file or
a directory.

Think about what needs to be written to the log to make this happen:

the parent directory's data (to add the new name->inum mapping)
the parent directory's inode (because you modified its data)
the portion of the inode map referring to the parent directory's inode
the new inode for the new file
the portion of the inode map referring to the new file's inode

In addition, if the new file is a directory, it should automatically get
the . and .. entries added to its contents, referring to its own inode
number and its parent's inode number, respectively. MFS_Creat does not
write any default data for regular files.

Note that MFS_Creat returns success if the file already exists. To
understand why this is the case, think about what should be the correct
behavior for the following sequence of operations:

the client sends an initial create request
the server receives and performs the request, creating the new file
the server sends its response, but the response is lost
the client times out and resends the request
		 */
		
		/*
	get next inode
	write parent dir
	get parent loc
	if type == file
		make file 
	 else // == dir
	 	write "." & ".." entries

		// write immediate entry:
		write data block
	 	update block pointer in inode
	 	write inode
	 	update inode pointer in imap
	 	write imap
	 	update checkpoint region with changes
	 	// update the parent directory
	 	write data block
	 	update block pointer in inode
	 	write inode
	 	update inode pointer in imap
	 	write imap
	 	update checkpoint region with changes
	 	write checkpoint region (once)
		 */
	return 0;
}

int
mkFile(int pinum, char* name, 
	   int inum, int location) {
  printf("Here to make a file in parent inode %d which is location at %d and with name %s\n with file iNode num %d", pinum, location, name, inum);
}

int
mkDir(int pinum, char* name, 
	  int inum, int location) {
  printf("Here to make a file in parent inode %d which is location at %d and with name %s\n with file iNode num %d", pinum, location, name, inum);
}

/**
 * Removes the file or directory name from the directory specified by pinum. 
 * 0 on success, -1 on failure. 
 * Failure modes: pinum does not exist, directory is NOT empty. Note that the 
 * name not existing is NOT a failure by our definition (we might getting
 * multiple requests that are the same...).
 */
int srv_Unlink(int pinum, char *name){
	printf("SERVER:: you called MFS_Unlink\n");
	int inum = pinum;
	// Check for invalid inum
	if(inum < 0 || inum >= MFS_BLOCK_SIZE) 
	  return -1;

	// read the checkpoint region
	int eol_ptr = 0; // the end of LFS pointer; should always point to an imap
	lseek(fd, 0, SEEK_SET);
	if(read(fd, &eol_ptr, sizeof(int)) < 0) { 
	  return -1; 
	}

	// Read in our 256 imap pointers... each with 16 inode refs; 4096 max inums
	int ckpr_ptrs[256];
	if(read(fd, &ckpr_ptrs, sizeof(int) * 256) < 0) { 
	  return -1; 
	}

	int imap_index = inum / 16;
	int imap_loc = ckpr_ptrs[imap_index];

	// We found our imap piece; now use modulus to find one of 16 inode refs
	int inode_index = inum % 16;
	// Each imapIdx is of 4 bytes; multiply by 4
	int imap_ptrs[16];
	lseek(fd, imap_loc, SEEK_SET);
	if(read(fd, &imap_ptrs, sizeof(int) * 16) < 0) { 
	  return -1; 
	}

	// Now read from the imap piece to find the inode_loc
	int inode_loc = imap_ptrs[inode_index];
	// Confirm the inode_loc is valid
	if (inode_loc <= 0 || inode_loc > eol_ptr) { 
	  return -1; 
	}

	// Read inode to determine size / type
	Inode_t* curr_inode = malloc(sizeof(Inode_t));
	lseek(fd, inode_loc, SEEK_SET);
	if (read(fd, curr_inode, sizeof(Inode_t)) < 0) { 
	  free(curr_inode); 
	  return -1; 
	}
	
	// We can only unlink a name from a directory
	if (curr_inode->type != MFS_DIRECTORY) { 
	  free(curr_inode); 
	  return -1; 
	}
	
	// Find the directory or file to unlink...
	int inode_ptrs[14];
	lseek(fd, (inode_loc + sizeof(Inode_t)), SEEK_SET);
	if (read(fd, &inode_ptrs, sizeof(int) * 14) < 0) { 
	  free(curr_inode); 
	  return -1; 
	}

	// Find the directory or file to remove
	MFS_DirEnt_t dir_entry;
	int is_found = 0;
	int dir_entry_block = -1;

	int i = 0;
	for (; i < 14; ++i) {
		if(inode_ptrs[i] == 0) 
		  continue; // inode not in use!

		// printf("inode:%d, loc:%d\n", i, inode_ptrs[i]);
		lseek(fd, inode_ptrs[i], SEEK_SET);
 
		int count = 0;
		while(count != 64) {
			++count;
			// Reading a directory entry moves the file pointer forward to the next
			if (read(fd, &dir_entry, sizeof(MFS_DirEnt_t)) < 0) { 
			  continue; 
			}

			// Ignore invalid inode numbers
			if (dir_entry.inum == -1) { 
			  continue; 
			}
			
			// printf("inum:%d, dir:%s\n", dir_entry.inum, dir_entry.name);
			if (strcmp(dir_entry.name, name) == 0) { 
				is_found = 1;  
				dir_entry_block = i;
				break; 
			} 
		}

		if (is_found) {
		  break;
		}
	}

	// If we couldn't find the name then our delete was a 'success'...
	if(!is_found) { 
	  free(curr_inode); 
	  return 0; 
	}

	int unlink_loc = get_inode_location(dir_entry.inum);

	if(unlink_loc <= 0) { // Fail.....
	  free(curr_inode); 
	  return -1; 
	} 

	// if(is_found) printf("FOUND!!!!!!! dir_entry.inum: %d \n", dir_entry.inum);

	// Determine whether this is a file or directory we are unlinking
	Inode_t unlink_inode;
	MFS_DirEnt_t unlink_dir;

	lseek(fd, unlink_loc, SEEK_SET); // This is one I have found I guess

	if (read(fd, &unlink_inode, sizeof(Inode_t)) < 0) { 
	  free(curr_inode); 
	  return -1; 
	}

	// This is a directory; if it's not empty we can't delete it
	if (unlink_inode.type == MFS_DIRECTORY) {  

		int unlink_ptrs[14];

		lseek(fd, (unlink_loc + sizeof(Inode_t)), SEEK_SET);
		if (read(fd, &unlink_ptrs, sizeof(int) * 14) < 0) { 
		  free(curr_inode); 
		  return -1; 
		}
		
		// Determine if the directory is empty... 
		for (i = 0; i < 14; ++i) {
		  if(unlink_ptrs[i] == 0) {
			  continue; // inode not in use!
		  }

		  // printf("directory: unlink_ptrs[%d]:%d\n", i, unlink_ptrs[i]);
		  lseek(fd, unlink_ptrs[i], SEEK_SET);
	 
		  int count = 0;
		  while(count != 64) {
			++count;
				
			// Reading a directory entry moves the file pointer forward to the next
			if (read(fd, &unlink_dir, sizeof(MFS_DirEnt_t)) < 0) { 
			  continue; 
			}
			
			// Ignore invalid inode numbers
			if (unlink_dir.inum == -1) { 
			  continue; 
			}
				
			// Always ignore the first two directory entries; "." and ".."
			if(count < 3) { 
			  continue; 
			}
				
			// If we reach this point, we know this directory is not empty... 
			// we can't delete a non-empty directory...
			printf("can't delete a non-empty directory.....\n");
			free(curr_inode); 
			return -1;
			// printf("directory: count:%d ulink: inum:%d, dir:%s\n", count, unlink_dir.inum, unlink_dir.name);
		  }
		}
	}

	// Wipe out the directory entry data that we no longer need
	MFS_DirEnt_t dir_entries[64];
	
	assert(dir_entry_block >= 0);

	lseek(fd, inode_ptrs[dir_entry_block], SEEK_SET);
	// Read in the current directory data block
	if (read(fd, &dir_entries, MFS_BLOCK_SIZE) < 0) { 
	  free(curr_inode);
	  return -1; 
	}

	// Modify the data block...
	int unlink_success = 0;

	for (i = 0; i < 64; ++i) {
		if(dir_entries[i].inum == -1) 
		  continue;
		
		// printf("dir_entries[%d]: dir_entry.inum:%d, dir_entry.name:%s \n", i, 
		// dir_entries[i].inum, dir_entries[i].name);
		if (strcmp(dir_entries[i].name, name) == 0) {
			// printf("entry to wipe!! inum:%d, dir:%s\n", dir_entry.inum, dir_entry.name);
			dir_entries[i].inum    = -1;
			dir_entries[i].name[0] = '\0';
			unlink_success = 1;
			break; 
		}
	}

	assert(unlink_success == 1);

	// Simple if both entries are in the same imap; a little more tricky if the
	// parent is in one imap & the directory was in another
	// Naive implementation: assume both inums are in the same imap piece
	// @todo: add edge-case logic to handle scenario when inums are not in the same imap piece!
	assert(imap_index == (dir_entry.inum / 16));

	int unlink_inode_index = dir_entry.inum % 16;

	// printf("unlink_inode_index:%d, inode_index:%d\n", unlink_inode_index, inode_index);

	// dir_entry.inum <-- set this inum location to zero in the parent imap
	// wipe out the parent directory entry by setting the inum to -1

	// 1-append the new data block
	lseek(fd, eol_ptr, SEEK_SET); // <--data goes to end of log...
	if (write(fd, dir_entries, MFS_BLOCK_SIZE) < 0) { 
	  free(curr_inode); 
	  return -1; 
	}

	// Record the new position of our data block
	inode_ptrs[dir_entry_block] = eol_ptr;
	// Set the eol_ptr just past the data block we just wrote (to the inode)
	eol_ptr += MFS_BLOCK_SIZE; // <--this is now our inode location...

	// 2-append the updated inode which points to the new data block
	if (write(fd, &curr_inode, sizeof(Inode_t)) < 0) { 
	  free(curr_inode); 
	  return -1; 
	}

	if (write(fd, &inode_ptrs, (sizeof(int) * 14)) < 0) { 
	  free(curr_inode); 
	  return -1; 
	}

	// Point the imap to our new inode
	imap_ptrs[inode_index] = eol_ptr;
	// Zero out the location to our inode which is no longer valid
	imap_ptrs[unlink_inode_index] = 0;
	eol_ptr += (sizeof(Inode_t) + (sizeof(int) * 14)); // Size of inode + ptrs
	// 3-append the imap which now points to the new inode
	imap_loc = eol_ptr; // <--need to point checkpoint region to imap piece
	if(write(fd, &imap_ptrs, sizeof(int) * 16) < 0) { 
	  return -1; 
	}

	// Update our imap location
	eol_ptr += (sizeof(int) * 16); // Size of imap

	// 4-updated the checkpoint region which now points to the new imap
	lseek(fd, (imap_index * 4) + 4, SEEK_SET);
	if (write(fd, &imap_loc, sizeof(int)) < 0) { 
	  return -1; 
	}

	// 5-update our end of log ptr to point to the new imap
	lseek(fd, 0, SEEK_SET);
	if (write(fd, &eol_ptr, sizeof(int)) < 0) { 
	  return -1; 
	}

	fsync(fd);
	free(curr_inode);
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
int srv_Shutdown(int rc, int sd, struct sockaddr_in s, struct msg_r* m){
	// Notify client we are shutting down; this is the only method completely 
	// tied to a client call.
	rc = UDP_Write(sd, &s, (char *) m, sizeof(struct msg_r)); 
	// @todo: we probably need to call fsync (or an equivalent) before exit!
	fs_Shutdown();
	// This will never happen....
	return 0;
}

int call_rpc_func(int rc, int sd, struct sockaddr_in s, struct msg_r* m){

	switch(m->method){
		case M_Init:
			sprintf(m->reply, "MFS_Init");
			srv_Init();
			break;
		case M_Lookup:
			sprintf(m->reply, "MFS_Lookup");
            printf ("Looking up with m->pinum %d and m->name %s\n", m->pinum, m->name);
			m->rc = srv_Lookup(m->pinum, m->name);
            printf ("In server received rc as = %d\n",m->rc);
			break;
		case M_Stat:
			sprintf(m->reply, "MFS_Stat");
			srv_Stat(m->inum, &(m->mfs_stat));
			break;
		case M_Write:
			sprintf(m->reply, "MFS_Write");
			srv_Write(m->inum, m->buffer, m->block);
			break;
		case M_Read:
			sprintf(m->reply, "MFS_Read");
			srv_Read(m->inum, m->buffer, m->block);
			break;
		case M_Creat:
			sprintf(m->reply, "MFS_Creat");
			srv_Creat(m->pinum, m->type, m->name);
			break;
		case M_Unlink:
			sprintf(m->reply, "MFS_Unlink");
			srv_Unlink(m->pinum, m->name);
			break;
		case M_Shutdown:
			sprintf(m->reply, "MFS_Shutdown");
			srv_Shutdown(rc, sd, s, m);
			break;
		default:
			printf("SERVER:: method does not exist\n");
			sprintf(m->reply, "Failed");
			break;
	}

	printf("reply: %s\n", m->reply);
	return UDP_Write(sd, &s, (char *) m, sizeof(struct msg_r)); 
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
    printf ("SERVER::m.rc = %d\n", m.rc);
		if (rc > 0) {
			rc = call_rpc_func(rc, sd, s, &m);
			printf("SERVER:: message %d bytes (message: '%s')\n", rc, m.buffer);
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
	// int inum;

	// srv_Init();

	// dump_log();
  
		// srv_Lookup(0, ".");
	// char buffer[MFS_BLOCK_SIZE];
	// sprintf(buffer, "#include <stdio.h>\nxxxxxxxxxx\n");
	// int rs = srv_Write(3, buffer, 0);

	// int rs = srv_Unlink(0, "dir");
	// int rs = srv_Unlink(6, "turtles"); // should fail
	// assert(rs >= 0);
	// dump_log();

	// srv_Creat(0, MFS_DIRECTORY, "awesome-dir");

	// MFS_Stat_t m;
	// srv_Stat(1, &m);
	// printf ("size of inode 1 is: %d\n",m.size);
	// printf ("type of inode 1 is: %d\n",m.type);
	// srv_Stat(0, &m);
	// printf ("size of inode 0 is: %d\n",m.size);
	// printf ("type of inode 0 is: %d\n",m.type);
	// srv_Stat(3, &m);
	// printf ("size of inode 3 is: %d\n",m.size);
	// printf ("type of inode 3 is: %d\n",m.type);
	// Can we find the base dir?
	// inum = srv_Lookup(0, "dir");
	// printf("/dir is inum: %d\n", inum); // we expect 1
	// inum = srv_Lookup(0, "code");
	// printf("/code is inum: %d\n", inum); // we expect 2
	// inum = srv_Lookup(0, "it's");
	// printf("/it's is inum: %d\n", inum); // we expect 6
	// inum = srv_Lookup(2, "helloworld.c");
	// printf("/helloworld is inum: %d\n", inum); // we expect 6
	// char* buffer = malloc(MFS_BLOCK_SIZE*sizeof(char));
	// int rc = srv_Read(3, buffer, 0);
	// if (rc == 0) {
	//   printf("From srv_Read\n");
	//   printf("%s\n", buffer);
	//   free(buffer);
	// }
	return 0;
}
