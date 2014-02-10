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
#include <malloc.h>

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

/**
 * Execute the provided command
 */
void 
execute(char** argv) 
{

  if (strcmp(argv[0], "cd") == 0) {
    char* home;
    
    if (argv[1] == NULL) {
      home = getenv("HOME");
    } else {
      home = argv[1];
    }
    
    if(chdir(home) < 0){
      fprintf(stderr, "%s is not a valid path\n", home);
    }
    return;
  }


  int rc = fork();

  if (rc > 0) { // This is the parent
     
    (void) wait(NULL); 
    // @todo: don't wait if there's an ampersand "&"

  } else if (rc == 0) { // This is the child
    
    if (strcmp(argv[0], "pwd") == 0) {
      char cwd[256];
      if (getcwd(cwd, sizeof(cwd)) == NULL) {
        // perror("pwd error!!!!"); // What should the error be?
      } else {
        printf("%s\n", cwd);
      }
      exit(0);
    } else if (strcmp(argv[0], "wait") == 0) {
      (void) wait(NULL); 
      exit(0);
    } else {
      /*This is for standard commands!
        We need to execute these commands 
        using exec as the spec says on the page.*/
      
      char* execStr;
      execStr = (char*)malloc(strlen(argv[0]) + strlen("/bin/") + 1);
      strcat(execStr, "/bin/");
      strcat(execStr, argv[0]);
      argv[0] = execStr;
      execvp(*argv, argv);
      printf("%s is not a recognized command\n", argv[0]);
      exit(0);
    }
    
    
    /*// Close the FD associated with stdout
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

      size_t length = strlen(line) - 1;
      if (line[length] == '\n')
        line[length] = '\0';

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
