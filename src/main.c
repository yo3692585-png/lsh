/***************************************************************************//**
  @file         main.c
  @author       Stephen Brennan (Modified by Youssef)
  @date         Thursday, 8 January 2015
  @brief        LSH (Libstephen SHell) with Custom Builtin Extensions
*******************************************************************************/

#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* --- History System Settings and Global States --- */
#define MAX_HISTORY 100
char *history_commands[MAX_HISTORY];
int history_count = 0;

/**
   @brief Adds a command line string to the history log buffer.
   @details Implements a fixed-size queue behavior using memory shifting 
            to prevent memory leaks when the buffer limit is reached.
   @param line The raw character string input entered by the user.
 */
void add_to_history(char *line) {
    // Edge Case: Skip empty entries, null pointers, or pure newlines
    if (line == NULL || strlen(line) == 0 || strcmp(line, "\n") == 0) return;
    
    // If the history buffer reaches its maximum capacity (100)
    if (history_count >= MAX_HISTORY) {
        // Free the memory of the oldest command at index 0
        free(history_commands[0]);
        
        // Shift all remaining commands one position to the left (1 -> 0, 2 -> 1, etc.)
        for (int i = 1; i < MAX_HISTORY; i++) {
            history_commands[i - 1] = history_commands[i];
        }
        
        // Insert the newest command safely at the final slot (index 99)
        history_commands[MAX_HISTORY - 1] = strdup(line);
    } else {
        // If buffer is not full, allocate memory and append to the array sequentially
        history_commands[history_count] = strdup(line);
        history_count++;
    }
}

/*
  Function Declarations for builtin shell commands:
 */
int lsh_cd(char **args);
int lsh_help(char **args);
int lsh_exit(char **args);
int lsh_pwd(char **args);
int lsh_echo(char **args);
int lsh_history(char **args);
int lsh_env(char **args);

/*
  List of builtin commands, followed by their corresponding functions.
  CRITICAL: Must contain all 7 functions to match declarations and prevent crashes!
 */
char *builtin_str[] = {
  "cd",
  "help",
  "exit",
  "pwd",
  "echo",
  "history",
  "env"
};

int (*builtin_func[]) (char **) = {
  &lsh_cd,
  &lsh_help,
  &lsh_exit,
  &lsh_pwd,
  &lsh_echo,
  &lsh_history,
  &lsh_env
};

/**
   @brief Get the number of registered builtin commands.
   @return Total count of builtin shell functions.
 */
int lsh_num_builtins() {
  return sizeof(builtin_str) / sizeof(char *);
}

/*
  Builtin function implementations.
*/

/**
   @brief Builtin command: change directory.
   @param args List of args. args[0] is "cd". args[1] is the target directory.
   @return Always returns 1, to continue execution loop.
 */
int lsh_cd(char **args)
{
  if (args[1] == NULL) {
    fprintf(stderr, "lsh: expected argument to \"cd\"\n");
  } else {
    if (chdir(args[1]) != 0) {
      perror("lsh");
    }
  }
  return 1;
}

/**
   @brief Builtin command: print shell help guidelines.
   @param args List of args. Not used.
   @return Always returns 1, to continue execution loop.
 */
int lsh_help(char **args)
{
  int i;
  printf("Youssef's LSH (Simple Shell)\n");
  printf("Type program names and arguments, and hit enter.\n");
  printf("The following are built in:\n");

  for (i = 0; i < lsh_num_builtins(); i++) {
    printf("  %s\n", builtin_str[i]);
  }

  printf("Use the man command for information on other programs.\n");
  return 1;
}

/**
   @brief Builtin command: exit and close the shell environment.
   @details Performs clean memory cleanup of the history array to eliminate leaks.
   @param args List of args. Not used.
   @return Always returns 0, to terminate execution lifecycle.
 */
int lsh_exit(char **args)
{
  // Safely free all allocated memory chunks inside the history matrix before terminating
  for (int i = 0; i < history_count; i++) {
    free(history_commands[i]);
  }
  return 0;
}

/**
   @brief Builtin command: print working directory (pwd).
   @details Dynamically retrieves the current path using POSIX system call getcwd().
   @param args List of args. Not used.
   @return Always returns 1, to continue execution loop.
 */
int lsh_pwd(char **args) {
    char cwd[1024]; // Allocate static buffer for path storage
    
    // Invoke getcwd system call and check if it successfully returns the absolute path
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("%s\n", cwd);
    } else {
        perror("lsh: pwd error"); // Output standard error if getcwd fails
    }
    return 1;
}

/**
   @brief Builtin command: echo arguments back to terminal.
   @details Loops through command string tokens starting from index 1.
   @param args Null-terminated array of tokens to be printed.
   @return Always returns 1, to continue execution loop.
 */
int lsh_echo(char **args) {
    int i = 1; // Skip args[0] because it holds the command name "echo" itself
    
    // Iterate through all tokens until the null-terminator is encountered
    while (args[i] != NULL) {
        printf("%s", args[i]);
        
        // Add a clean dividing space only if there is an additional trailing argument
        if (args[i+1] != NULL) {
            printf(" ");
        }
        i++;
    }
    printf("\n"); // Print trailing newline character for proper formatting
    return 1;
}

/**
   @brief Builtin command: history log printer.
   @details Outputs sequentially numbered lines representing stored inputs.
   @param args List of args. Not used.
   @return Always returns 1, to continue execution loop.
 */
int lsh_history(char **args) {
    // Loop through the log and display elements prefixed with sequential numbers
    for (int i = 0; i < history_count; i++) {
        printf(" %d  %s", i + 1, history_commands[i]);
        
        // Dynamic formatting fix: append newline if command was captured without one
        if (history_commands[i][strlen(history_commands[i])-1] != '\n') {
            printf("\n");
        }
    }
    return 1;
}

/**
   @brief Builtin command: display system environment variables (env).
   @details Iterates through the Linux core environment array mapping variable values.
   @param args List of args. Not used.
   @return Always returns 1, to continue execution loop.
 */
int lsh_env(char **args) {
    extern char **environ; // Import global pointer array reference to external environment data
    
    // Sequential loop through environment entries until the terminal NULL pointer is hit
    for (int i = 0; environ[i] != NULL; i++) {
        printf("%s\n", environ[i]);
    }
    return 1;
}

/**
  @brief Launch an external program and wait for its termination.
  @param args Null-terminated list of arguments (including program).
  @return Always returns 1, to continue execution loop.
 */
int lsh_launch(char **args)
{
  pid_t pid;
  pid_t wpid;
  int status;

  pid = fork();
  if (pid == 0) {
    // Child process
    if (execvp(args[0], args) == -1) {
      perror("lsh");
    }
    exit(EXIT_FAILURE);
  } else if (pid < 0) {
    // Error forking
    perror("lsh");
  } else {
    // Parent process
    do {
      wpid = waitpid(pid, &status, WUNTRACED);
    } while (!WIFEXITED(status) && !WIFSIGNALED(status));
  }

  return 1;
}

/**
   @brief Execute a specific shell built-in function or launch program.
   @param args Null-terminated list of tokenized command arguments.
   @return 1 if the shell session continues, 0 if it should terminate.
 */
int lsh_execute(char **args)
{
  int i;

  if (args[0] == NULL) {
    return 1;
  }

  for (i = 0; i < lsh_num_builtins(); i++) {
    if (strcmp(args[0], builtin_str[i]) == 0) {
      return (*builtin_func[i])(args);
    }
  }

  return lsh_launch(args);
}

/**
   @brief Read a single comprehensive line of input from stdin.
   @return Dynamically allocated line stream from standard input channel.
 */
char *lsh_read_line(void)
{
#ifdef LSH_USE_STD_GETLINE
  char *line = NULL;
  ssize_t bufsize = 0; 
  if (getline(&line, &bufsize, stdin) == -1) {
    if (feof(stdin)) {
      exit(EXIT_SUCCESS); 
    } else  {
      perror("lsh: getline\n");
      exit(EXIT_FAILURE);
    }
  }
  return line;
#else
#define LSH_RL_BUFSIZE 1024
  int bufsize = LSH_RL_BUFSIZE;
  int position = 0;
  char *buffer = malloc(sizeof(char) * bufsize);
  int c;

  if (!buffer) {
    fprintf(stderr, "lsh: allocation error\n");
    exit(EXIT_FAILURE);
  }

  while (1) {
    c = getchar();

    if (c == EOF) {
      exit(EXIT_SUCCESS);
    } else if (c == '\n') {
      buffer[position] = '\0';
      return buffer;
    } else {
      buffer[position] = c;
    }
    position++;

    if (position >= bufsize) {
      bufsize += LSH_RL_BUFSIZE;
      buffer = realloc(buffer, bufsize);
      if (!buffer) {
        fprintf(stderr, "lsh: allocation error\n");
        exit(EXIT_FAILURE);
      }
    }
  }
#endif
}

#define LSH_TOK_BUFSIZE 64
#define LSH_TOK_DELIM " \t\r\n\a"
/**
   @brief Split an inputted continuous line into separate string tokens.
   @param line The continuous raw string line.
   @return Null-terminated array of dynamic sub-string tokens.
 */
char **lsh_split_line(char *line)
{
  int bufsize = LSH_TOK_BUFSIZE;
  int position = 0;
  char **tokens = malloc(bufsize * sizeof(char*));
  char *token;
  char *line_copy;

  if (!tokens) {
    fprintf(stderr, "lsh: allocation error\n");
    exit(EXIT_FAILURE);
  }

  line_copy = strdup(line);
  token = strtok(line_copy, LSH_TOK_DELIM);
  while (token != NULL) {
    tokens[position] = strdup(token);
    position++;

    if (position >= bufsize) {
      bufsize += LSH_TOK_BUFSIZE;
      tokens = realloc(tokens, bufsize * sizeof(char*));
      if (!tokens) {
        fprintf(stderr, "lsh: allocation error\n");
        exit(EXIT_FAILURE);
      }
    }

    token = strtok(NULL, LSH_TOK_DELIM);
  }
  tokens[position] = NULL;
  free(line_copy);
  return tokens;
}

/**
   @brief The primary execution loop driving the shell application.
 */
void lsh_loop(void)
{
  char *line;
  char **args;
  int status;

  do {
    printf("> ");
    line = lsh_read_line();
    
    // Save input stream explicitly into history buffer before parsing tokenized objects
    add_to_history(line);
    
    args = lsh_split_line(line);
    status = lsh_execute(args);

    // Clean execution memory blocks at the loop terminal stage to prevent leaks
    free(line);
    if (args != NULL) {
        for (int i = 0; args[i] != NULL; i++) {
            free(args[i]);
        }
        free(args);
    }
  } while (status);
}

/**
   @brief Main program entry point.
 */
int main(int argc, char **argv)
{
  lsh_loop();
  return EXIT_SUCCESS;
}