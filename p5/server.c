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

// Checkpoint region (we should never have more than one of these!)
typedef struct __Checkpoint_region_t {
	int eol_ptr; // End of Log pointer
	// Our 256 imap pointers... each with 16 inode refs; 4096 max inums
	int ckpr_ptrs[256];
} Checkpoint_region_t;

// Current imap & inode
typedef struct __Idata_t {
	int imap_ptrs[16];
	int imap_index;
	int imap_loc;
	int inode_index;
	int inode_loc;
	Inode_t curr_inode;
	int inode_ptrs[14];
} Idata_t;

int read_cr(Checkpoint_region_t *cr){
	// read the checkpoint region
	lseek(fd, 0, SEEK_SET);
	if(read(fd, &cr->eol_ptr, sizeof(int)) < 0) { 
		return -1; 
	}

	if(read(fd, &cr->ckpr_ptrs, sizeof(int) * 256) < 0) { 
		return -1; 
	}

	return 0;
}

// Requires read_cr first!
int read_imap(int inum, Checkpoint_region_t *cr, Idata_t *c){
	c->imap_index = inum / 16;
	c->imap_loc = cr->ckpr_ptrs[c->imap_index];
	c->inode_index = inum % 16;

	// Get imap
	lseek(fd, c->imap_loc, SEEK_SET);
	if(read(fd, &c->imap_ptrs, sizeof(int) * 16) < 0) {
		return -1; 
	}

	// Get position of inode
	c->inode_loc = c->imap_ptrs[c->inode_index];

	// Don't do this test; it breaks stuff if this is an imap for a NEW inodet
	// that doesn't exist yet.
	// if (c->inode_loc <= 0 || c->inode_loc > cr->eol_ptr) { 
	// 	return -1; 
	// }

	// int j, curr_inum;
	// for(j = 0; j < 16; j++){
	// 	curr_inum = (j + (c->imap_index * 16));
	// 	printf("imap_ptrs[%d]:%d inum:%d, imap piece#:%d \n", j, c->imap_ptrs[j], curr_inum, c->imap_index);
	// 	// If the checkpoint region location is zero, the inum is free
	// }
	// exit(0);

	return 0;
}

// Requires read_imap first!
int read_inode(Checkpoint_region_t *cr, Idata_t *c){
	// Get inode struct
	lseek(fd, c->inode_loc, SEEK_SET);
	if (read(fd, &c->curr_inode, sizeof(Inode_t)) < 0) { 
		return -1; 
	}

	// Get inode block pointers
	lseek(fd, (c->inode_loc + sizeof(Inode_t)), SEEK_SET);
	if (read(fd, &c->inode_ptrs, sizeof(int) * 14) < 0) { 
		return -1; 
	}

	// int i;
	// for (i = 0; i < 14; i++){
	// 	printf("inode_blk_ptrs[%i]:%d\n",i, c->inode_ptrs[i]);
		
	// }
	// exit(0);
	return 0;
}

// Expects an imap was just written
int write_cr(Checkpoint_region_t *cr, Idata_t *c){
	// updated the checkpoint region which now points to the new imap
	lseek(fd, (c->imap_index * 4) + 4, SEEK_SET);
	if (write(fd, &c->imap_loc, sizeof(int)) < 0) { 
		return -1; 
	} // We are now done with the child.

	// printf("CR Region update: c->imap_loc: %d\n", c->imap_loc);

	// Update the EOL pointer
	lseek(fd, 0, SEEK_SET);
	if (write(fd, &cr->eol_ptr, sizeof(int)) < 0) { 
		return -1; 
	}

	return 0;
}

// Expects an inode was just written
int write_imap(Checkpoint_region_t *cr, Idata_t *c){
	lseek(fd, cr->eol_ptr, SEEK_SET);
	if(write(fd, &c->imap_ptrs, sizeof(int) * 16) < 0) { 
		return -1; 
	}
	// Our new imap piece loc which we will put in the checkpoint region
	c->imap_loc = cr->eol_ptr;
	// printf("cr->eol_ptr / c->imap_loc: %d\n", cr->eol_ptr);
	cr->eol_ptr += (sizeof(int) * 16); // Size of imap
	return 0;
}

// Write inode, point imap to it, update eol_ptr
int write_inode(Checkpoint_region_t *cr, Idata_t *c){
	// Write our inode to the EOL:
	lseek(fd, cr->eol_ptr, SEEK_SET);
	// Do I need &?
	if (write(fd, &c->curr_inode, sizeof(Inode_t)) < 0) { 
		return -1; 
	}
	if (write(fd, &c->inode_ptrs, (sizeof(int) * 14)) < 0) { 
		return -1; 
	}

	// Update our imap & EOL
	c->imap_ptrs[c->inode_index] = cr->eol_ptr;
	// printf("c->imap_ptrs[c->inode_index] / imap_ptr: %d\n", cr->eol_ptr);
	cr->eol_ptr += (sizeof(Inode_t) + (sizeof(int) * 14)); // Size of inode + ptrs

	return 0;
}

// Writes data, point inode to it, update eol_ptr
int write_dir(Checkpoint_region_t *cr, Idata_t *c, int block, MFS_DirEnt_t *d){
	if(block < 0 || block > 13){
		return -1;
	}
	lseek(fd, cr->eol_ptr, SEEK_SET);
	if(write(fd, d, MFS_BLOCK_SIZE) < 0) { 
		return -1; 
	}
	// Update the inode with our new block location
	c->inode_ptrs[block] = cr->eol_ptr;
	// Shift the end of log pointer behind the new data...
	cr->eol_ptr += MFS_BLOCK_SIZE;
	return 0;
}

// Writes data, point inode to it, update eol_ptr
int write_file(Checkpoint_region_t *cr, Idata_t *c, int block, char *d){
	if(block < 0 || block > 13){
		return -1;
	}
	lseek(fd, cr->eol_ptr, SEEK_SET);
	if(write(fd, d, MFS_BLOCK_SIZE) < 0) { 
		return -1; 
	}
	c->inode_ptrs[block] = cr->eol_ptr;
	cr->eol_ptr += MFS_BLOCK_SIZE;
	return 0;
}

// Requires read_cr first!
int get_next_inode(Checkpoint_region_t *cr){

	// Loop through the 256 checkpoint region pointers
	Idata_t temp;
	int i;
	int j;
	int inum = -1;
	int is_found = 0;
	for (i = 0; i < 256; i++){
		if(is_found) break;
		// Read in the imap piece
		lseek(fd, cr->ckpr_ptrs[i], SEEK_SET);
		if(read(fd, &temp.imap_ptrs, sizeof(int) * 16) < 0) { 
			return -1; 
		}
		// Check each of the 16 pointers in the imap piece
		for(j = 0; j < 16; j++){
			inum = (j + (i * 16));
			// printf("ckpr_ptrs[%d]:%d imap_ptrs[%d]:%d inum:%d, imap piece#:%d \n", i, cr->ckpr_ptrs[i],j,temp.imap_ptrs[j], inum, i);
			// If the checkpoint region location is zero, the inum is free
			if(temp.imap_ptrs[j] == 0) {
				is_found = 1;
				break;
			}
		}
	}

	if(! is_found) {
		return -1;
	}
	return inum;
}

// Dump contents of the current file
void dump_file_inode(int fd, int file_loc, int file_size){
	int inode_blk_ptrs[14];
	int i = 0;
	char buffer[MFS_BLOCK_SIZE];
	if(file_size <= 0){ 
		printf("file_loc:%d (empty file)\n", file_loc);
		return;
	}
	lseek(fd, (file_loc + sizeof(Inode_t)), SEEK_SET);
	if (read(fd, &inode_blk_ptrs, sizeof(int) * 14) < 0) { return; }
	// Read dir inode structure: 14x pointers to 64 byte directory blocks
	for (; i < 14; ++i) {
		if(inode_blk_ptrs[i] == 0) continue; // inode not in use!
		// printf("block ptr:%d, loc:%d\n", i, inode_blk_ptrs[i]);
		lseek(fd, inode_blk_ptrs[i], SEEK_SET);
		// printf("inode_blk_ptrs[%i]:%d, data block[%d] contents: \n%s\n",i, inode_blk_ptrs[i], buffer);
		while(file_size > 0) {
			printf("curr file size: %d\n", file_size);
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
			printf("inum:%d, name:%s\n", dirEntry.inum, dirEntry.name);
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
	for (; i < 256; i++){
		// Make sure we read from the right position...
		lseek(fd, i * 4, SEEK_SET);
		rc = read(fd, &ckpt_rg, sizeof(int));
		if(i == 0){
			printf("1st checkpoint ptr: %d\n", ckpt_rg);
			continue;
		}
		if(ckpt_rg == 0) continue;
		// prev = ckpt_rg;
		// printf("ckpt_rg %d: %d, ckpt_rg - prev:%d\n", i, ckpt_rg, ckpt_rg - prev);
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
			// if(inode_loc == 0) continue;
			// printf("inode_loc: %d, ckpt_rg: %d, inum: %d, j: %d, i: %d \n", inode_loc, (ckpt_rg - 64) + (j * 4), the_inum, j, i);
			printf("inode_loc: %d, ckpt_rg: %d, inum: %d, imap piece#: %d \n", inode_loc, (ckpt_rg - 64) + (j * 4), the_inum, the_imap);
			// continue;
			lseek(fd, inode_loc, SEEK_SET);
			// continue;
			Inode_t curr_inode;
			if (read(fd, &curr_inode, sizeof(Inode_t)) < 0) { continue; }
			// Parent inode numbers can only belong to directories
			if (curr_inode.type != MFS_DIRECTORY) { // It's a file
				// printf("inode_loc:%d, file size:%d\n", inode_loc, curr_inode->size);
				dump_file_inode(fd, inode_loc, curr_inode.size);
			} else { // It's a directory
				dump_dir_inode(fd, inode_loc);
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

// int init_disk(){
// 	int i, j, inum, rs;
// 	fd = open(filename, O_RDWR | O_CREAT, S_IRWXU);
// 	if (fd < 0) {
// 		printf("Open error\n");
// 		exit(1);
// 	}

// 	Checkpoint_region_t cr;
// 	// Set EOL just past the CR
// 	cr.eol_ptr = (4 + 256*sizeof(int));
// 	// init CR to zero
// 	i = 0;
// 	for (; i < 256; ++i) {
// 		cr.ckpr_ptrs[i] = 0;
// 	}

// 	Idata_t c;
// 	c.imap_index = 0;
// 	c.imap_loc = (4 + 256*4 + 4096 + sizeof(Inode_t) + 14*sizeof(int));
// 	c.inode_index = 0;

// 	// Set up imap
// 	i = 0;
// 	for (; i < 16; i++) {
// 		c.imap_ptrs[i] = 0;
// 	}

// 	// Set up inode
// 	c.curr_inode.type = MFS_DIRECTORY;
// 	c.curr_inode.size = 4096;
// 	// Set the other inode block pointers to zero
// 	i = 0;
// 	for (; i < 14; i++) {
// 		c.inode_ptrs[i] = 0;
// 	}

// 	// Set up directory entries
// 	MFS_DirEnt_t dir_entries[64];
// 	j = 0;
// 	for (; i < 64; i++) {
// 		dir_entries[i].name[0] = '\0';
// 		dir_entries[i].inum    = -1;
// 	}

// 	dir_entries[0].name[0] = '.';
// 	dir_entries[0].name[1] = '\0';
// 	dir_entries[0].inum    = 0;
// 	dir_entries[1].name[0] = '.';
// 	dir_entries[1].name[1] = '.';
// 	dir_entries[1].name[2] = '\0';
// 	dir_entries[1].inum    = 0;

// 	rs = write_dir(&cr, &c, 0, &dir_entries[0]);
// 	assert(rs != -1);

// 	// Write the new & empty inode
// 	rs = write_inode(&cr, &c);
// 	assert(rs != -1);

// 	// Write the imap
// 	rs = write_imap(&cr, &c);
// 	assert(rs != -1);
// 	// update child end ---->

// 	// Update the checkpoint region
// 	rs = write_cr(&cr, &c);
// 	assert(rs != -1);

// 	return 0;
// }

int initDisk() {
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

	// Checkpoint_region_t cr;
	// int rs = read_cr(&cr);
	// i = 0;
	// for (; i < 256; ++i) {
	// 	printf("CHECKPOINT PTR[%d]%d\n", i, cr.ckpr_ptrs[i]);
	// }

	return 0;
}
 
int srv_Init() {
	// printf("SERVER:: you called MFS_Init\n");
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
	// printf("SERVER:: you called MFS_Lookup\n");
	// Ignore invalid parent inode numbers
	if (pinum < 0 || pinum > 4095) {
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

	// printf("SERVER:: you called MFS_Read\n");

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
		if (read(fd, buffer, MFS_BLOCK_SIZE) < 0) {
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
	// printf("SERVER:: you called MFS_Stat\n");
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
	// printf("SERVER:: you called MFS_Write\n");
	if(block < 0 || block > 13 || inum < 0 || inum >= MFS_BLOCK_SIZE) {
		return -1;
	}

	int rs;

	Checkpoint_region_t cr;
	rs = read_cr(&cr);
	if(rs == -1) return -1;

	// Read in the imap
	Idata_t c;
	rs = read_imap(inum, &cr, &c);
	assert(rs != -1);

	// Read in the inode
	rs = read_inode(&cr, &c);
	assert(rs != -1);

	if (c.curr_inode.type == MFS_DIRECTORY) { 
		return -1; 
	}
	
	int data_size = strlen(buffer);
	// Add the new buffer to the total file size
	int file_size = data_size;
	char tmp_buffer[MFS_BLOCK_SIZE];
	int tmp_size = 0;
	int i = 0;
	for (; i < 14; i++){
		// This is the block we are replacing; ignore it.
		if (i == block || c.inode_ptrs[i] <= 0){
			continue;
		}
		// file_size += 4096;
		lseek(fd, c.inode_ptrs[i], SEEK_SET);
		if (read(fd, &tmp_buffer, MFS_BLOCK_SIZE) < 0 ) { 
			return -1; 
		}
		tmp_size = strlen(tmp_buffer);
		if(tmp_size > 0 && tmp_size <=4096){
			file_size += strlen(tmp_buffer);
		} else {
			file_size += 4096;
		}
	}

	c.curr_inode.size = file_size;
	assert(c.curr_inode.size >= 0 && c.curr_inode.size <= MFS_BLOCK_SIZE * 14);
	// printf("new size: %d\n", curr_inode->size);

	rs = write_file(&cr, &c, 0, buffer);
	assert(rs != -1);

	// Write the new & empty inode
	rs = write_inode(&cr, &c);
	assert(rs != -1);

	// Write the imap
	rs = write_imap(&cr, &c);
	assert(rs != -1);

	// Update the checkpoint region
	rs = write_cr(&cr, &c);
	assert(rs != -1);

	fsync(fd);
	return 0;
} 

/**
 * Makes a file (type == MFS_REGULAR_FILE) or directory (type == MFS_DIRECTORY) 
 * in the parent directory specified by pinum of name *name. Returns 0 on 
 * success, -1 on failure. Failure modes: pinum does not exist, or name is too 
 * long. If name already exists, return success.
 */
int srv_Creat(int pinum, int type, char *name){
	// printf("SERVER:: you called MFS_Creat\n");
	int i,j;

	// Make sure the name is not too long
	int len = strlen(name);
	if(len > 59){
		return -1;
	}

	// Success if the name already exists
	int rs = srv_Lookup(pinum, name);
	if (rs >= 0) {
		return 0;
	}

	// The parent must be a dir
	MFS_Stat_t pstat;
	srv_Stat(pinum, &pstat);
	if(pstat.type != MFS_DIRECTORY){
		return -1;
	}

	// Read in the checkpoint region
	Checkpoint_region_t cr;
	rs = read_cr(&cr);
	if(rs == -1) return -1;

	// Read in the parent imap
	Idata_t p;
	rs = read_imap(pinum, &cr, &p);
	assert(rs != -1);

	// Read in the parent inode
	rs = read_inode(&cr, &p);
	assert(rs != -1);

	// Determine the inode to assign
	int inum = get_next_inode(&cr);
	assert(inum > 0);

	// Set up our new inode & imap struct
	Idata_t c;

	// Read in the imap for the new item
	rs = read_imap(inum, &cr, &c);
	assert(rs != -1);

	// Set the inode data
	c.curr_inode.type = type;
	// If this is a file, size == 0
	c.curr_inode.size = 0;
	// If this is a dir, size == 4096
	if (type == MFS_DIRECTORY) {
		c.curr_inode.size = MFS_BLOCK_SIZE;
	}
	// Set our inode pointers to zero
	for (i = 0; i < 14; i++) {
		c.inode_ptrs[i] = 0;
	}

	// Find dir entry start -->
	// Always use the first free directory entry
	MFS_DirEnt_t dir_entries[64];
	int dir_entry_block = -1;
	for (i = 0; i < 14; i++) {
		if(dir_entry_block >= 0) {
			break;
		}
		// We found an empty directory block first; use it.
		if(p.inode_ptrs[i] == 0) {
			// Init directory to zero; but make this our first entry
			strcpy(dir_entries[0].name, name);
			dir_entries[0].inum = inum;
			// Initialize the rest of this directory to empty items...
			for(j = 1; j < 64; j++){
				dir_entries[j].inum    = -1;
				dir_entries[j].name[0] = '\0';
			}
			// Which inode block ptr did we use? This is kind of important.. 
			dir_entry_block = i;
			break;
		}
		// Read the current in-use directory block
		lseek(fd, p.inode_ptrs[i], SEEK_SET);
		if (read(fd, &dir_entries, MFS_BLOCK_SIZE) < 0) { 
			return -1; 
		}
		int count = 0;
		while(count < 64) {
			if (dir_entries[count].inum == -1) { 
				// We found an available directory entry; add our new item
				strcpy(dir_entries[count].name, name);
				dir_entries[count].inum = inum;
				dir_entry_block = i;
				break;
			}
			count++;
		}
	}
	// The directory must have been full; we can't continue.
	if(dir_entry_block < 0){
		return -1;
	}
	// <-- Find dir entry end

	// <--- update child start
	if (type == MFS_DIRECTORY) {
	// Write a data block for the new directory
		MFS_DirEnt_t new_dir[64];
		new_dir[0].name[0] = '.';
		new_dir[0].name[1] = '\0';
		new_dir[0].inum    = inum;
		new_dir[1].name[0] = '.';
		new_dir[1].name[1] = '.';
		new_dir[1].name[2] = '\0';
		new_dir[1].inum    = pinum;

		int i = 2;
		for (; i < 64; i++) {
			new_dir[i].name[0] = '\0';
			new_dir[i].inum    = -1;
		}

		rs = write_dir(&cr, &c, 0, &new_dir[0]);
		assert(rs != -1);
	}

	// Write the new & empty inode
	rs = write_inode(&cr, &c);
	assert(rs != -1);

	// Write the imap
	rs = write_imap(&cr, &c);
	assert(rs != -1);
	// update child end ---->

	// Update the checkpoint region
	rs = write_cr(&cr, &c);
	assert(rs != -1);

	if(c.imap_index == p.imap_index){
		// Sync things up since we will only be writing one imap.... 
		p.imap_loc = c.imap_loc;
	}

	// <--- update parent start
	rs = write_dir(&cr, &p, dir_entry_block, &dir_entries[0]);
	assert(rs != -1);

	rs = write_inode(&cr, &p);
	assert(rs != -1);

	// If the child & parent are in the same imap piece...
	if(c.imap_index == p.imap_index){
		// Update the parent imap piece with the new child inode location
		p.imap_ptrs[c.inode_index] = c.imap_ptrs[c.inode_index];
	}
	rs = write_imap(&cr, &p);
	assert(rs != -1);

	// Update the checkpoint region
	rs = write_cr(&cr, &p);
	assert(rs != -1);
	// update parent end ---->

	// int curr_inum;
	// printf("CURR IMAP:\n");
	// for(j = 0; j < 16; j++){
	// 	curr_inum = (j + (c.imap_index * 16));
	// 	printf("imap_ptrs[%d]:%d inum:%d, imap piece#:%d \n", j, c.imap_ptrs[j], curr_inum, c.imap_index);
	// 	// If the checkpoint region location is zero, the inum is free
	// }
	// printf("PARENT IMAP:\n");
	// for(j = 0; j < 16; j++){
	// 	curr_inum = (j + (p.imap_index * 16));
	// 	printf("imap_ptrs[%d]:%d inum:%d, imap piece#:%d \n", j, p.imap_ptrs[j], curr_inum, p.imap_index);
	// 	// If the checkpoint region location is zero, the inum is free
	// }

	fsync(fd);
	return 0;
}

/**
 * Removes the file or directory name from the directory specified by pinum. 
 * 0 on success, -1 on failure. 
 * Failure modes: pinum does not exist, directory is NOT empty. Note that the 
 * name not existing is NOT a failure by our definition (we might getting
 * multiple requests that are the same...).
 */
int srv_Unlink(int pinum, char *name){
	// printf("SERVER:: you called MFS_Unlink\n");
	// Check for invalid inum
	if(pinum < 0 || pinum >= MFS_BLOCK_SIZE) 
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

	int pimap_index = pinum / 16;
	int pimap_loc = ckpr_ptrs[pimap_index];

	// We found our imap piece; now use modulus to find one of 16 inode refs
	int pinode_index = pinum % 16;

	// Each imapIdx is of 4 bytes; multiply by 4
	int pimap_ptrs[16];
	lseek(fd, pimap_loc, SEEK_SET);
	if(read(fd, &pimap_ptrs, sizeof(int) * 16) < 0) { 
		return -1; 
	}

	// Now read from the imap piece to find the inode_loc
	int pinode_loc = pimap_ptrs[pinode_index];
	// Confirm the inode_loc is valid
	if (pinode_loc <= 0 || pinode_loc > eol_ptr) { 
		return -1; 
	}

	// Read inode to determine size / type
	Inode_t curr_pinode;
	lseek(fd, pinode_loc, SEEK_SET);
	if (read(fd, &curr_pinode, sizeof(Inode_t)) < 0) { 
		return -1; 
	}
	
	// We can only unlink a name from a directory
	if (curr_pinode.type != MFS_DIRECTORY) { 
		return -1; 
	}
	
	// Find the directory or file to unlink...
	int pinode_ptrs[14];
	lseek(fd, (pinode_loc + sizeof(Inode_t)), SEEK_SET);
	if (read(fd, &pinode_ptrs, sizeof(int) * 14) < 0) { 
		return -1; 
	}

	// Find the directory or file to remove
	MFS_DirEnt_t dir_entry;
	int is_found = 0;
	int dir_entry_block = -1;

	int i = 0;
	for (; i < 14; ++i) {
		if(pinode_ptrs[i] == 0) {
			continue; // inode not in use!
		}

		// printf("inode:%d, loc:%d\n", i, inode_ptrs[i]);
		lseek(fd, pinode_ptrs[i], SEEK_SET);
 
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
		return 0; 
	}

	int unlink_loc = get_inode_location(dir_entry.inum);

	if(unlink_loc <= 0) { // Fail.....
		return -1; 
	} 

	// if(is_found) printf("FOUND!!!!!!! dir_entry.inum: %d \n", dir_entry.inum);

	// Determine whether this is a file or directory we are unlinking
	Inode_t unlink_inode;
	MFS_DirEnt_t unlink_dir;

	lseek(fd, unlink_loc, SEEK_SET); // This is one I have found I guess

	if (read(fd, &unlink_inode, sizeof(Inode_t)) < 0) { 
		return -1; 
	}

	// This is a directory; if it's not empty we can't delete it
	if (unlink_inode.type == MFS_DIRECTORY) {  

		int unlink_ptrs[14];

		lseek(fd, (unlink_loc + sizeof(Inode_t)), SEEK_SET);
		if (read(fd, &unlink_ptrs, sizeof(int) * 14) < 0) { 
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
			// printf("can't delete a non-empty directory.....\n");
			return -1;
			// printf("directory: count:%d ulink: inum:%d, dir:%s\n", count, unlink_dir.inum, unlink_dir.name);
			}
		}
	}

	// Wipe out the directory entry data that we no longer need
	MFS_DirEnt_t dir_entries[64];
	
	assert(dir_entry_block >= 0 && dir_entry_block < 14);

	lseek(fd, pinode_ptrs[dir_entry_block], SEEK_SET);
	// Read in the current directory data block
	if (read(fd, &dir_entries, MFS_BLOCK_SIZE) < 0) { 
		return -1; 
	}

	// Modify the data block...
	int unlink_success = 0;

	for (i = 0; i < 64; ++i) {
		if(dir_entries[i].inum == -1) {
			continue;
		}
		
		// printf("dir_entries[%d]: dir_entry.inum:%d, dir_entry.name:%s \n", i, dir_entries[i].inum, dir_entries[i].name);
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
	assert(pimap_index == (dir_entry.inum / 16));

	int unlink_inode_index = dir_entry.inum % 16;

	// printf("unlink_inode_index:%d, inode_index:%d\n", unlink_inode_index, inode_index);

	// dir_entry.inum <-- set this inum location to zero in the parent imap
	// wipe out the parent directory entry by setting the inum to -1

	// 1-append the new data block
	lseek(fd, eol_ptr, SEEK_SET); // <--data goes to end of log...
	if (write(fd, &dir_entries, MFS_BLOCK_SIZE) < 0) { // ???????????????????
		return -1; 
	}

	// Record the new position of our data block
	pinode_ptrs[dir_entry_block] = eol_ptr;
	// Set the eol_ptr just past the data block we just wrote (to the inode)
	eol_ptr += MFS_BLOCK_SIZE; // <--this is now our inode location...

	// 2-append the updated inode which points to the new data block
	if (write(fd, &curr_pinode, sizeof(Inode_t)) < 0) { 
		return -1; 
	}

	if (write(fd, &pinode_ptrs, (sizeof(int) * 14)) < 0) { 
		return -1; 
	}

	// Point the imap to our new inode
	pimap_ptrs[pinode_index] = eol_ptr;
	// Zero out the location to our inode which is no longer valid
	pimap_ptrs[unlink_inode_index] = 0;
	eol_ptr += (sizeof(Inode_t) + (sizeof(int) * 14)); // Size of inode + ptrs
	// 3-append the imap which now points to the new inode
	pimap_loc = eol_ptr; // <--need to point checkpoint region to imap piece
	if(write(fd, &pimap_ptrs, sizeof(int) * 16) < 0) { 
		return -1; 
	}

	// Update our imap location
	eol_ptr += (sizeof(int) * 16); // Size of imap

	// 4-updated the checkpoint region which now points to the new imap
	lseek(fd, (pimap_index * 4) + 4, SEEK_SET);
	if (write(fd, &pimap_loc, sizeof(int)) < 0) { 
		return -1; 
	}

	// 5-update our end of log ptr to point to the new imap
	lseek(fd, 0, SEEK_SET);
	if (write(fd, &eol_ptr, sizeof(int)) < 0) { 
		return -1; 
	}

	fsync(fd);
	return 0;
}

void fs_Shutdown(){
	int rs = fsync(fd);
	assert(rs != -1);
	rs = close(fd);
	assert(rs != -1);
	// printf("SERVER:: you called MFS_Shutdown\n");
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
	m->rc = 0;// success!
	rc = UDP_Write(sd, &s, (char *) m, sizeof(struct msg_r)); 
	// @todo: we probably need to call fsync (or an equivalent) before exit!
	fs_Shutdown();
	// This will never happen....
	return 0;
}

int call_rpc_func(int rc, int sd, struct sockaddr_in s, struct msg_r* m){

	// printf ("SERVER::rpc_func got m as %p\n",m);
	switch(m->method){
		case M_Init:
			sprintf(m->reply, "MFS_Init");
			m->rc = srv_Init();
			break;
		case M_Lookup:
			sprintf(m->reply, "MFS_Lookup");
			// printf("SERVER:: says I am in lookup case\n");
			// printf ("SERVER:: side name %s\n",m->name);
			// if (m->name != NULL) {
			// 	printf ("Looking up with m->pinum %d and m->name %s\n", m->pinum, m->name);
			// } else {
			// 	printf ("Got NULL\n");
			// }
			m->rc = srv_Lookup(m->pinum, m->name);
			// printf ("In server received rc as = %d\n",m->rc);
			break;
		case M_Stat:
			sprintf(m->reply, "MFS_Stat");
			m->rc = srv_Stat(m->inum, &(m->mfs_stat));
			break;
		case M_Write:
			sprintf(m->reply, "MFS_Write");
			m->rc = srv_Write(m->inum, m->buffer, m->block);
			break;
		case M_Read:
			sprintf(m->reply, "MFS_Read");
			m->rc = srv_Read(m->inum, m->buffer, m->block);
			break;
		case M_Creat:
			sprintf(m->reply, "MFS_Creat");
			m->rc = srv_Creat(m->pinum, m->type, m->name);
			break;
		case M_Unlink:
			sprintf(m->reply, "MFS_Unlink");
			m->rc = srv_Unlink(m->pinum, m->name);
			break;
		case M_Shutdown:
			sprintf(m->reply, "MFS_Shutdown");
			srv_Shutdown(rc, sd, s, m);
			break;
		default:
			printf("SERVER:: method does not exist\n");
			sprintf(m->reply, "Failed");
			// m->rc = -1;
			break;
	}

	// printf("reply: %s\n", m->reply);
	return UDP_Write(sd, &s, (char *) m, sizeof(struct msg_r)); 
}

void start_server(int argc, char *argv[]){
	int sd, port; 

	getargs(&port, argc, argv); 
	sd = UDP_Open(port); 
	assert(sd > -1);

	// printf("SERVER:: waiting in loop\n");

	while (1) { 
		struct sockaddr_in s; 
		struct msg_r m; 
		int rc = UDP_Read(sd, &s, (char *) &m, sizeof(struct msg_r));
		// printf ("SERVER::m.rc = %d\n", m.rc);
		// printf ("SERVER::m.name = %s\n",m.name);
		if (rc > 0) {
			rc = call_rpc_func(rc, sd, s, &m);
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
	// int inum;

	// filename = "new.img";
	// srv_Init();
	// int rs = srv_Creat(0, MFS_REGULAR_FILE, "test");
	// assert(rs == 0);
	// rs = srv_Write(1, "START BLOCK 1", 0);
	// assert(rs == 0);
	// char buffer[MFS_BLOCK_SIZE];
	// rs = srv_Read(1, (char*) &buffer, 0);
	// assert(rs == 0);
	// printf("buffer: %s\n", buffer);
	// assert(strcmp(buffer, "START BLOCK 1") == 0);
	// printf("test passed!\n");

	// MFS_Stat_t m;
	// rs = srv_Stat(1, &m);
	// assert(rs == 0);
	// printf ("size of inode 1 is: %d\n",m.size);
	// printf ("type of inode 1 is: %d\n",m.type);

	// dump_log();

	// srv_Lookup(0, ".");
	// sprintf(buffer, "#include <stdio.h>\nxxxxxxxxxx\n");
	// int rs = srv_Write(3, buffer, 0);

	// int rs = srv_Unlink(6, "turtles"); // should fail
	// int rs = srv_Unlink(0, "dir");
	// assert(rs >= 0);

	// int rs = srv_Creat(0, MFS_DIRECTORY, "awesome-dir");
	// int rs = srv_Creat(0, MFS_REGULAR_FILE, "awesome!!");
	// assert(rs == 0);
	// dump_log();

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
