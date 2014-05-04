#include <stdio.h>
#include <sys/select.h>
#include "udp.h"
#include "mfs.h"

char *name = "tempfile";
int pinum, type, inum, block, response;
char buffer[4096];
MFS_Stat_t *my_dir;
MFS_Stat_t *my_file;

void test_init(){
  response = MFS_Init("localhost", 10000);
  assert(response >= 0);
  // printf("MFS_Init passed\n");  
}

void test_creat(){
  response = MFS_Creat(pinum, type, name);
  assert(response >= 0);
  // printf("MFS_Creat passed\n");
}

void test_write(){
  response = MFS_Write(inum, buffer, block);
  assert(response >= 0);
  // printf("MFS_Write passed\n");
}

void test_read(){
  response = MFS_Read(inum, buffer, block);
  assert(response >= 0);
  // printf("MFS_Read passed\n");
}

void test_stat(){
  response = MFS_Stat(inum, my_file);
  assert(response >= 0);
  // printf("MFS_Stat passed\n");
}

void test_lookup(){
  response = MFS_Lookup(pinum, name);
  assert(response >= 0);
  // printf("MFS_Lookup passed\n");
}

void test_unlink(){
  response = MFS_Unlink(pinum, name);
  assert(response >= 0);
  // printf("MFS_Unlink passed\n");
}

void test_shutdown(){
  response = MFS_Shutdown();
  assert(response >= 0);
  // printf("MFS_Shutdown passed\n");
}

/**
 *  Test each respective client method..
 */
int main(int argc, char *argv[]){

  // generic values (to make the compiler happy)
  my_dir = malloc(sizeof(*my_dir));
  my_file = malloc(sizeof(*my_file));
  my_dir->type = MFS_DIRECTORY;
  my_dir->size = 64;
  my_file->type = MFS_DIRECTORY;
  my_file->size = 64;
  pinum = 1;
  type = 1;
  inum = 1;
  block = 1;
  sprintf(buffer, "An awesome message");
  // Confirm the server sends a success response in each case
  test_init();
  test_creat();
  test_write();
  test_read();
  test_stat();
  test_lookup();
  test_unlink();
  test_shutdown();
  printf("All client tests passed\n");

   // int sd=atoi(argv[1]);
   // /* int sd = UDP_Open(0); */
   // /* assert(sd > -1); */

   //  struct sockaddr_in saddr;

   //  int rc = MFS_Init(argv[2],sd);
   //  assert(rc==0);

   //  char message[BUFFER_SIZE];
   //  sprintf(message, "hello world");

   //  return 0;
}

