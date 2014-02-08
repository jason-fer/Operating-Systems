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
#include <string.h>

void
usage(char *prog) 
{
  fprintf(stderr, "Usage: fastsort -i inputfile -o outputfile\n");
  exit(1);
}

void 
shell() 
{
  printf("mysh> ");
}

void 
execute(char* line) 
{
  int rc = fork();

  if (rc > 0) {
    (void) wait(NULL);
  } else if (rc == 0) {
    char* argv[2];

      if (strcmp(line,"ls") == 0) {
        argv[0] = "/bin/ls";
        argv[1] = NULL;
        execvp(*argv, argv);
      } else if (strcmp(line, "exit") == 0) {
        exit(0);
      }

  } else {
    perror("Error in Forking!\n");
    exit(1);
  }
}

int
main(int argc, char *argv[]) 
{
  char line[512];
  while (1) {
    shell();
    scanf("%s", line); /* for now will change it to something else*/
    execute(line);
  }
  return 0;
}
