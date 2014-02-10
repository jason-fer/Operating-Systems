/**
 * Mysh
 * Emulate the unix shell with fork(), exec(), wait(), etc.
 */

#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAX_LINE_SIZE 512

/**
 * Let the user know we are ready for input
 */
void 
prompt() 
{
  printf("mysh> ");
}

/**
 * Compare two strings; ignore whitespace. First strips all white-space, then
 * return the comparison. 
 */

void
tokenize(char* line,
         char** token)
{
  int i = 0;
  token[i++] = strtok(line, " ");

  while (token[i-1] != NULL) {
    token[i++] = strtok(NULL, " ");
  }
}

/* int  */
/* strcmpigwhite(const char *str1, const char *str2) */
/* { */
/*   int x; */
/*   char *pstr1; */
/*   char *pstr2; */
/*   int max1 = strlen(str1); */
/*   int max2 = strlen(str2); */
/*   int i = 0; */

/*   // Add +1 for the null pointer */
/*   pstr1 = malloc(max1 + 1); */
/*   pstr2 = malloc(max2 + 1); */

/*   // @todo: check malloc */

/*   // Strip all white space characters from string 1 */
/*   x = 0; */
/*   for (; i < max1; ++i) { */

/*    if(str1[i] == '\0'){ */
/*     // Found null; terminate the string & break the loop */
/*     pstr1[x] = str1[i]; */
/*     break; */
/*    } else if (! isspace(str1[i])) { */
/*     // Copy over the non-whitespace item */
/*     pstr1[x] = str1[i]; */
/*     x++; */
/*    } */
/*   } */

/*   // Strip all white space characters from string 2 */
/*   x = 0; */
/*   for (i = 0; i < max2; ++i) { */

/*    if(str2[i] == '\0'){ */
/*     // Found null; terminate the string & break the loop */
/*     pstr2[x] = str2[i]; */
/*     break; */
/*    } else if (! isspace(str2[i])) { */
/*     // Copy over the non-whitespace item */
/*     pstr2[x] = str2[i]; */
/*     x++; */
/*    } */
/*   } */

/*   return strcmp(pstr1, pstr2); */
/* } */

/**
 * Execute the provided command
 */
void 
execute(char** argv) 
{
  int rc = fork();

  if (rc > 0) { // This is the parent
    
    (void) wait(NULL); 
    // @todo: don't wait if there's an ampersand "&"

  } else if (rc == 0) { // This is the child
    
    /*Check for build in commands -- do we need to fork on these?*/
    if (strcmp(argv[0], "pwd") == 0) {
      char cwd[256];
      if (getcwd(cwd, sizeof(cwd)) == NULL) {
        // perror("pwd error!!!!"); // What should the error be?
      } else {
        printf("%s\n", cwd);
      }
    } else if (strcmp(argv[0], "cd") == 0) {
      const char* home = getenv("HOME");
      if(chdir(home) < 0){
        // perror("cd error!!!!"); // What should the error be?
      }
    } else if (strcmp(argv[0], "wait") == 0) {
      (void) wait(NULL); 
    } /*else if (somefunc(argv[0], "cd <somedir>")){
      // [optionalSpace]cd[oneOrMoreSpace]dir[optionalSpace]
      // if (strcmp(argv[0], "cd dir")) // needs special handling
    }*/

    // Standard commands probably need completely different handling.
    /* char* argv[2];
    if (strcmp(argv[0], "ls") == 0) {
      argv[0] = "/bin/ls";
      argv[1] = NULL;
      execvp(*argv, argv);
    }*/

    // Code from Daniel Myers (I guess Remzi gave very similar code)
    /*char *exec_args[4];
    exec_args[0] = "ls";
    exec_args[1] = "-l";
    exec_args[2] = "-a";
    exec_args[3] = NULL;

    // Close the FD associated with stdout
    int close_rc = close(STDOUT_FILENO);
    if (close_rc < 0) {
      perror("close");
      exit(1);
    }

    // // Open a new file
    int fd = open("outfile.txt", O_RDWR | O_TRUNC, S_IRWXU);
    if (fd < 0) {
      perror("open");
      exit(1);
    }
    printf("File descriptor number of new opened file = %d\n", fd);
    printf("File descriptor number of stdout = %d\n", STDOUT_FILENO);

    execvp("ls", exec_args);
    printf("If you're reading this something went wrong!\n");*/

  } else if(rc < 0){ // Failed to fork

    fprintf(stderr, "fork() failed\n");
    exit(1);

  }
}

/**
 * Prompt the user, parse the input and pass it to execute.
 */
int
main(int argc, char *argv[]) 
{
  char line[MAX_LINE_SIZE];

  while (1) {

    prompt();

    if(fgets(line, MAX_LINE_SIZE, stdin)){
      
      char* token[20];
      tokenize(line, token);

      if (strcmp(token[0], "exit") == 0) {
        // Exit must be done outside the forked process
        exit(0);
      } else {
        execute(token);
      }
    }
  }

  return 0;
}
