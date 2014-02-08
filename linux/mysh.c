/**
 * Mysh
 * <insert description here>
 *
 * Usage
 * shell% ./fastsort -i inputfile -o outputfile
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

void
usage(char *prog) 
{
  fprintf(stderr, "Usage: fastsort -i inputfile -o outputfile\n");
  exit(1);
}

void shell() {
  printf("mysh> \n");
}

int
main(int argc, char *argv[]) {

  int rc = fork();

  if (rc == 0) {
    shell();
    (void) wait(NULL);
  } else if (rc >= 0) {
    printf ("I am here\n");
    while(1) {
      ;
    }
  } else {
    perror("Error in Forking!\n");
  }

  return 0;
}
