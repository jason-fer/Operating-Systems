#include "types.h"
#include "user.h"

int
main(int argc, char *argv[])
{

  // This should throw an error
  int *p = NULL;
  printf(1, "NULL Pointer addr: 0x%x\n", p);
  printf(1, "De-reference Null Pointer: %d\n", *p);
  printf(1, "De-reference Null Pointer: 0x%x\n", *p);

  /*
  // Top of the user addr space, minus first page
  char *p = (char *) ((640 * 1024) - 4096);
  // print addr as unsigned hex
  printf(1, "addr of p: %x\n", p);
  // attempt to write some data in this space
  *p = 'f';

  // confirm the address space gets copied correctly
  int rc = fork();
  if(rc == 0){
    // Will trap because child wont get copy of high page
    printf(1, "child: %c\n", *p);
  } else if (rc > 0){
    (void) wait();
    printf(1, "parent: %c\n", *p);
  } else {
    printf(1, "bad thing happened");
  }
  printf(1, "%c\n", *p);
  */
 
  exit();
}