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
#include <errno.h>

// The maximum line size in bytes
#define MAX_LINE_SIZE 512

/**
 * Let the user know we are ready for input
 */
void 
prompt() 
{
  write(STDOUT_FILENO,"mysh> ", strlen("mysh> "));
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
  token[i++] = strtok(line, " ");

  if(token[0] != NULL){
    while (token[i-1] != NULL) {
      token[i++] = strtok(NULL, " ");
    }
  }
}

/**
 * Should the child process run in the background? Detect whether the final 
 * argument was an ampersand "&". Returns "1" if an ampersand is detected, 
 * and "0" otherwise.
 */
int
runChildInBG(char** token)
{
  int i = 0;
  char *last = NULL;

  while(token[i] != NULL) {
    last = token[i++];
  }

  if(last != NULL && last[strlen(last) - 1] == '&') {
    if (strcmp(last, "&") == 0) {
      token[i - 1] = NULL;
    } else {
      token[i - 1][strlen(last) - 1] = '\0';
    }
    // Ampersand detected: the child should run in the background
    return 1;
  }
  
  // Ampersand not detected: the child should not run in the background
  return 0;
}

/**
 * Checks whether redirection needs to be done. If there is an ">" it returns 
 * the file name. Also modifies the arguments accordingly. Else returns NULL.
 */
char*
doRedirection(char** token)
{
  char line[MAX_LINE_SIZE];
  int i = 1;
  
  if (token[0] == NULL) {
    return strdup("\0");
  }
  
  strcpy(line, token[0]);
  
  while(token[i] != NULL) {
    strcat(line, token[i]);
    ++i;
  } 
  
  int countRedirects = 0;
  int pos;
  for (pos = 0; line[pos]; ++pos) {
    if (line[pos] == '>') {
      ++countRedirects;
    }
  }

  if (countRedirects > 1) {
    error();
    exit(1);
  }

  char* filePos = strchr(line, '>');

  if (!filePos) {
    return strdup("\0");
  }

  char* searchingLastTok = strchr(token[i-1], '>');
  char* penultimateTok = NULL;

  if (i > 2) {
    penultimateTok = strchr(token[i-2], '>');
  }

  if (searchingLastTok || penultimateTok) {

    int found = 0;
    int j = 0;
    while (token[j]) {
      if (strchr(token[j],'>') || found) {
        found = 1;
        if(strchr(token[j],'>')){
          // Check for edge case where we don't want to delete the token.
          int position = (int) strcspn(token[j], ">");
          if (position >= 1) {
            // This token starts with an argument we need to keep.
            token[j][position] = '\0';
          } else {
            // This token is safe to delete
            token[j] = NULL;
          }
        } else {
          // This token is safe to delete
          token[j] = NULL;
        }
      }
      j++;
    }

    return (strdup(filePos+1));
  } else {
    error();
    exit(-1);
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
      error(); // "home" is not a valid path
    }
    return;
  }

  // This must be done before we fork since it strips the ampersand.
  int runChildProcInBg = runChildInBG(argv);
  int rc = fork();
  
  if (rc > 0) { // This is the parent

    // The child process sould run in the background if an "&" ampersand is 
    // passed is as the final parameter.
    if (runChildProcInBg == 0){
      int stat;
      // Wait because no final ampersand "&" parameter was detected
      int pid = waitpid(rc, &stat, WUNTRACED);
      if (pid < 0) {
        error();
      }
    } 
    return;
  } else if (rc == 0) { // This is the child
    
    char* fileName = doRedirection(argv);
    
    // Remove ampersand if it exists
    if (fileName[strlen(fileName) - 1] == '&') {
      fileName[strlen(fileName) - 1] = '\0';
    }

    if (strcmp(fileName, "\0") != 0) {
      printf("Opening fileName %s\n", fileName);
      int close_rc = close(STDOUT_FILENO);
      if (close_rc < 0) {
        error(); // Error closing file
        exit(1);
      }

      // Open a new file
      int fd = open(fileName, O_RDWR | O_TRUNC | O_CREAT, S_IRWXU);
 
      if (fd < 0) {
        error(); // Error opening file
        exit(1);
      }
    }

    if (strcmp(argv[0], "pwd") == 0) {
      if(argv[1] != NULL){ error(); exit(0); } // Prevent additional arguments

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

      if (strcmp(&argv[0][strlen(argv[0])-3], ".py") == 0) {
        // Our "fun" feature... pass arguments to the python interpreter
        char* exec_args[20];
        exec_args[0] = "/usr/bin/python";
        int i;
        for (i = 0; i < 19; ++i) {
          exec_args[i + 1] = argv[i];  
        }
        exec_args[i + 1] = NULL;

        execvp(*exec_args, exec_args);
      } else {
        /* For standard commands, which we represent exactly as the user 
        specified, without modification. */
        execvp(*argv, argv);
      }

      error(); // We got an unrecognized command in argv[0]
      exit(1);
    }
    
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
  // A few bytes past the maximum length allow us to easily check for overflow
  char line[MAX_LINE_SIZE + 4];

  char* fileName;
  FILE* fp = NULL;

  // Batch file handling
  if (argc == 2) {
    fileName = argv[1];
    fp = fopen(fileName, "r");
    if (!fp) {
      error();
      exit(1);
    }
  } else if (argc > 2) {
    error();
    exit(1);
  } else {
    fp = stdin;
  }

  // Infinite read loop (until the user exits)
  while (1) {

    if (argc == 1) {
      prompt();
    }

    if (fgets(line, MAX_LINE_SIZE + 3, fp)) {

      size_t length = strlen(line) - 1;

      // Batch mode handling
      if (argc == 2) {
        // A greater than maximum length line
        if (strlen(line) - 1  > MAX_LINE_SIZE) {
          char *tmpline = strdup(line);
          // Show the user a truncated version of the text
          write(STDOUT_FILENO, tmpline, strlen(line));
          error();
          continue;
        } else {
          // Default
          write(STDOUT_FILENO, line, strlen(line));
        }
      }

      // Replace new line with a null-terminator
      if (line[length] == '\n'){
        line[length] = '\0';
      }

      // Tokenize the string into a series of commands / parameters
      char* token[20];
      tokenize(line, token);

      // Prevent the program from crashing if the user only enters whitespace
      if(token[0] == NULL) continue;
      
      if (strcmp(token[0], "exit") == 0) {
        // We only expect one argument
        if(token[1] != NULL){ error(); exit(0); }
        // Exit must be done outside the forked process
        exit(0);
      } else if(strcmp(token[0], "wait") == 0) {
        // We only expect one argument
        if(token[1] != NULL){ error(); exit(0); }

        // If we fork, then this 'wait' will not work as expected
        int stat;
        int done = 0;
        while (done != -1) {
          done = waitpid(-1, &stat, P_ALL);
        }

        if (done == -1) {
          if (errno == ECHILD) continue; // no more child processes
          else error();
        }

      } else {
        // Run the user request
        execute(token);
      }
    } else if (argc == 2) {       // Batch file handling
      fclose(fp);
      exit(0);
    } else {
      exit(0);
    }
  }

  return 0;
}
