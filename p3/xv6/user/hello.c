/**
 * Project 3B bounds test (I'm not sure why it's not passing) -Jason
 */
/* syscall argument checks (null page) */
#include "types.h"
#include "stat.h"
#include "user.h"
#include "fs.h"
#include "fcntl.h"
#include "syscall.h"
#include "traps.h"

int
main(int argc, char *argv[])
{
  char *arg;

  int fd = open("tmp", O_WRONLY|O_CREATE);
  // assert(fd != -1);
  if(fd == -1){
    printf(1, "assert failed (%s)\n", "fd != -1");
    printf(1, "TEST FAILED\n");
    exit();
  }

  printf(1, "File opened\n");

  // at zero
  arg = (char*) 0x0;
  // assert(write(fd, arg, 10) == -1);
  if(write(fd, arg, 10) != -1){
    printf(1, "assert failed (%s)\n", "write(fd, arg, 10)");
    printf(1, "TEST FAILED\n");
    exit();
  }

  printf(1, "File written 1\n");

  // within null page
  arg = (char*) 0x400;
  // assert(write(fd, arg, 1024) == -1);
  if(write(fd, arg, 1024) != -1){
    printf(1, "assert failed (%s)\n", "write(fd, arg, 1024)");
    printf(1, "TEST FAILED\n");
    exit();
  }

  printf(1, "File written 2\n");
   // spanning null page and code 
  arg = (char*) 0xfff;
  // assert(write(fd, arg, 2) == -1);
  if(write(fd, arg, 2) != -1){
    printf(1, "assert failed (%s)\n", "");
    printf(1, "TEST FAILED\n");
    exit();
  }

  printf(1, "File written 3\n");
  // ensure stack is actually high...
  //assert((uint) &local > 639*1024);

  printf(1, "TEST PASSED\n");
  exit();
}
