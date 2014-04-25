#include <stdio.h>
#include "udp.h"
#include "mfs.h"
/**
 * The client library: the methods below were defined here:
 * http://pages.cs.wisc.edu/~dsmyers/cs537/Projects/p5a.html
 *
 * We will need to wrap all commands in a struct with the key data required
 * for the server to carry out each respective command.
 *
 * RPC Stubs:
 */
int sd, rc;
struct sockaddr_in saddr, raddr;

int read_reply(int rc, int sd, struct msg_r *m, char *expected){
  assert(rc >= 0);
  printf("CLIENT:: sent message (%d)\n", rc);

  fd_set r;
  FD_ZERO(&r);
  FD_SET(sd, &r);
  struct timeval t; // timeout
  t.tv_sec = 5; // seconds
  t.tv_usec = 0; // microseconds
  rc = select(sd + 1, &r, NULL, NULL, &t);

  if (rc > 0){
    rc = UDP_Read(sd, &raddr, (char *) m, sizeof(struct msg_r)); // This blocks
    printf("CLIENT:: read %d bytes (message: '%s')\n", rc, m->reply);
    if(strcmp(expected, m->reply) == 0){ 
      return 0;
    } else {
      return -1;
    }
  }

  printf("CLIENT:: TIMED OUT\n");
  return -1;
}

/**
 * Takes a host name and port number and uses those to find the server exporting
 * the file system.
 */
int MFS_Init(char *hostname, int port){
  // Things we only need to do once...
  sd = UDP_Open(0);
  assert(sd > -1);
  // Once a socket is filled it can be re-used over & over
  rc = UDP_FillSockAddr(&saddr, hostname, port);
  assert(rc == 0);

  // Things we will need to do over & over...
  struct msg_r m;
  m.method = M_Init;
  // sprintf(m.buffer, "An awesome message");
  rc = UDP_Write(sd, &saddr, (char *) &m, sizeof(struct msg_r));
  // We expect the reply to contain "MFS_Init"
  return read_reply(rc, sd, &m, "MFS_Init");
}

/**
 * Makes a file (type == MFS_REGULAR_FILE) or directory (type == MFS_DIRECTORY) 
 * in the parent directory specified by pinum of name name . Returns 0 on 
 * success, -1 on failure. Failure modes: pinum does not exist, or name is too 
 * long. If name already exists, return success (think about why).
 */
int MFS_Creat(int pinum, int type, char *name){
  struct msg_r m;
  // sprintf(m.buffer, "An awesome message");
  m.method = M_Creat;
  rc = UDP_Write(sd, &saddr, (char *) &m, sizeof(struct msg_r));
  // We expect the reply to contain "MFS_Creat"
  return read_reply(rc, sd, &m, "MFS_Creat");
}

/**
 * Takes the parent inode number (which should be the inode number of a 
 * directory) and looks up the entry name in it. The inode number of name is 
 * returned. Success: return inode number of name; failure: return -1. Failure 
 * modes: invalid pinum, name does not exist in pinum.
 */
int MFS_Lookup(int pinum, char *name){
  struct msg_r m;
  // sprintf(m.buffer, "An awesome message");
  m.method = M_Lookup;
  rc = UDP_Write(sd, &saddr, (char *) &m, sizeof(struct msg_r));
  // We expect the reply to contain "MFS_Lookup"
  return read_reply(rc, sd, &m, "MFS_Lookup");
}

/**
 * Returns some information about the file specified by inum. Upon success, 
 * return 0, otherwise -1. The exact info returned is defined by MFS_Stat_t. 
 * Failure modes: inum does not exist.
 */
int MFS_Stat(int inum, MFS_Stat_t *mfs_stat){
  struct msg_r m;
  // sprintf(m.buffer, "An awesome message");
  m.method = M_Stat;
  rc = UDP_Write(sd, &saddr, (char *) &m, sizeof(struct msg_r));
  // We expect the reply to contain "MFS_Stat"
  return read_reply(rc, sd, &m, "MFS_Stat");
}

/**
 * Writes a block of size 4096 bytes at the block offset specified by block. 
 * Returns 0 on success, -1 on failure. Failure modes: invalid inum, invalid 
 * block, not a regular file (because you can't write to directories).
 */
int MFS_Write(int inum, char *buffer, int block){
  struct msg_r m;
  // sprintf(m.buffer, "An awesome message");
  m.method = M_Write;
  rc = UDP_Write(sd, &saddr, (char *) &m, sizeof(struct msg_r));
  // We expect the reply to contain "MFS_Write"
  return read_reply(rc, sd, &m, "MFS_Write");
} 

/**
 * Reads a block specified by block into the buffer from file specified by inum.
 * The routine should work for either a file or directory; directories should 
 * return data in the format specified by MFS_DirEnt_t. Success: 0, failure: -1.
 * Failure modes: invalid inum, invalid block.
 */
int MFS_Read(int inum, char *buffer, int block){
  struct msg_r m;
  // sprintf(m.buffer, "An awesome message");
  m.method = M_Read;
  rc = UDP_Write(sd, &saddr, (char *) &m, sizeof(struct msg_r));
  // We expect the reply to contain "MFS_Read"
  return read_reply(rc, sd, &m, "MFS_Read");
}

/**
 * Removes the file or directory name from the directory specified by pinum. 
 * 0 on success, -1 on failure. 
 * Failure modes: pinum does not exist, directory is NOT empty. Note that the 
 * name not existing is NOT a failure by our definition (think about why this 
 * might be).
 */
int MFS_Unlink(int pinum, char *name){
  struct msg_r m;
  // sprintf(m.buffer, "An awesome message");
  m.method = M_Unlink;
  rc = UDP_Write(sd, &saddr, (char *) &m, sizeof(struct msg_r));
  // We expect the reply to contain "MFS_Read"
  return read_reply(rc, sd, &m, "MFS_Unlink");
}

/**
 * Just tells the server to force all of its data structures to disk and 
 * shutdown by calling exit(0). This interface will mostly be used for testing 
 * purposes.
 */
int MFS_Shutdown(){
  struct msg_r m;
  // sprintf(m.buffer, "An awesome message");
  m.method = M_Shutdown;
  rc = UDP_Write(sd, &saddr, (char *) &m, sizeof(struct msg_r));
  // We expect the reply to contain "MFS_Read"
  return read_reply(rc, sd, &m, "MFS_Shutdown");
}
