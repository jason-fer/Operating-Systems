/**
 * This is the client library to access the server
 */
#ifndef __MFS_h__
#define __MFS_h__

#define MFS_DIRECTORY    (0)
#define MFS_REGULAR_FILE (1)

#define MFS_BLOCK_SIZE   (4096)
#define BUFFER_SIZE      (4096)

typedef struct __MFS_Stat_t {
    int type;   // MFS_DIRECTORY or MFS_REGULAR
    int size;   // bytes
    // note: no permissions, access times, etc.
} MFS_Stat_t;

typedef struct __Inode_t {
    int size;
    int type;
} Inode_t;

typedef struct __MFS_DirEnt_t {
    char name[60];  // up to 60 bytes of name in directory (including \0)
    int  inum;      // inode number of entry (-1 means entry not used)
} MFS_DirEnt_t;

// For the RPC stub client send & server recieve methods 
enum methods { 
  M_Init,
  M_Lookup,
  M_Stat,
  M_Write,
  M_Read,
  M_Creat,
  M_Unlink,
  M_Shutdown
};

struct msg_r {
  enum methods method;
  char *name;
  int pinum, inum, block, type;
  char buffer[4096];
  char reply[16];
  MFS_Stat_t mfs_stat;
};

int MFS_Init(char *hostname, int port);
int MFS_Lookup(int pinum, char *name);
int MFS_Stat(int inum, MFS_Stat_t *m);
int MFS_Write(int inum, char *buffer, int block);
int MFS_Read(int inum, char *buffer, int block);
int MFS_Creat(int pinum, int type, char *name);
int MFS_Unlink(int pinum, char *name);
int MFS_Shutdown();

#endif // __MFS_h__
