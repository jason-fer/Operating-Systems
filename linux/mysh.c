/**
 * Mysh
 * Emulate the unix shell behavior with fork(), exec(), wait(), etc. Allows
 * some basic commands, as well as redirection. Also allows the "&" paramater
 * to run a child process in the background.
 */

#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
//#include <malloc.h>

#define MAX_LINE_SIZE 512

volatile int bgprocess = 0;

/**
 * Let the user know we are ready for input
 */
void 
prompt() 
{
  printf("mysh> ");
}

/**
 * Standard error message for all errors
 */
void
error()
{
  char error_message[30] = "An error has occurred\n";
  write(STDERR_FILENO, error_message, strlen(error_message));
}

/**
 * Translate the line into a series of tokens which represent the provided 
 * commands / parameters.
 */
void
tokenize(char* line, char** token)
{
  int i = 0;
  char *last = NULL;

  token[i++] = strtok(line, " ");

  if(token[0] != NULL){
    while (token[i-1] != NULL) {
      last = token[i-1];
      token[i++] = strtok(NULL, " ");
    }
  }

  // Determine if this process should run in the background
  bgprocess = 0;
  if(last != NULL && last[strlen(last)-1] == '&'){
    bgprocess = 1;
  }
  
}

/**
 * Execute the tokenized commands / parameters
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
    
    // If bgprocess == 1, then we detected a final argument of ampersand "&"
    if(bgprocess != 1){
      (void) wait(NULL);
    }

    if(strcmp(argv[0], "wait") == 0){
      (void) wait(NULL);
    }
    
  } else if (rc == 0) { // This is the child
    
    if (strcmp(argv[0], "pwd") == 0) {
      char cwd[256];
      if (getcwd(cwd, sizeof(cwd)) == NULL) {
        error();
        exit(1);
      } else {
        printf("%s\n", cwd);
      }
      exit(0);
    } else if (strcmp(argv[0], "wait") == 0) {
      // Nothing to do (this directive is for the parent process)
      exit(0);
    } else {
      /* For standard commands, which we represent exactly as the user 
      specified, without modification. */
      
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

      // Replace new line with a null-terminator
      size_t length = strlen(line) - 1;
      if (line[length] == '\n'){
        line[length] = '\0';
      }

      // Tokenize the string into a series of commands / parameters
      char* token[20];
      tokenize(line, token);

      // Prevent the program from crashing if the user only enters whitespace
      if(token[0] == NULL) continue;

      if (strcmp(token[0], "exit") == 0) {
        // Exit must be done outside the forked process
        exit(0);
      } else {
        // Run the user request
        execute(token);
      }
    }
  }

  return 0;
}
