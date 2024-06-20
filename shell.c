#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#define CMDLINE_MAX 512
#define ARG_MAX 16
#define TOKEN_MAX 32

enum { REGULAR_CMD, BULLETIN_CMD, PIPING_CMD };

struct cmdline {
  char *p_cmd;             // parse cmd
  char *args[CMDLINE_MAX]; // array of arguments strings
  char *original_cmd_holder;
  char *filename;
  int redirect_flag;
  int background;
};

struct node {
  struct cmdline data;
  struct node *next;
  int status;
  int pid;
};

struct background_job {
  char *original_cmd_holder;
  int pid;
  struct node *pipe_list;
};

int pwd() {
  char path[CMDLINE_MAX];
  if (getcwd(path, CMDLINE_MAX) != NULL) {
    fprintf(stdout, "%s\n", path);
    return 0;
  } else {
    return 1;
  }
}

int cd(char **cmd) {
  // calculate the path
  char *path;
  path = (char *)malloc(strlen(cmd[1]) + 1);
  strcpy(path, cmd[1]);
  if (chdir(path)) {
    return 0;
  } else {
    fprintf(stderr, "Invalid path\n");
  }
  free(path);
  return 0;
}

void appendLinkedlist(struct node *head, struct cmdline cmd) {
  if (head->data.p_cmd == NULL) {
    head->data = cmd;
    head->next = NULL;
  } else {
    struct node *newNode = (struct node *)malloc(sizeof(struct node));
    newNode->data = cmd;
    newNode->next = NULL;
    struct node *iterator = head;
    while (iterator->next != NULL) {
      iterator = iterator->next;
    }
    iterator->next = newNode;
  }
  return;
}

// check if a cmd requires piping
int check_pipe(char *cmd) {
  if (strchr(cmd, '|') != NULL) {
    return 0;
  }
  return 1;
}

// error_management
int check_errors(char *cmd) {
  // make a copy of original cmdline
  char *temp_cmd = strdup(cmd);
  char *temp_args[ARG_MAX];
  int temp_argc = 0;
  int rd_flag = 0;

  // check for max char
  int cmd_len = strlen(cmd);
  if (cmd_len > CMDLINE_MAX) {
    fprintf(stderr, "Error: cmd exceeded 512 characters\n");
    return 1;
  }

  // parse cmdline into char pointer array
  // also check for argument size and max number of arguments
  char *token = strtok(temp_cmd, " ");
  while (token != NULL) {
    if ((int)strlen(token) > TOKEN_MAX) {
      fprintf(stderr, "Error: token or argument exceeded 32 characters\n");
      return 1;
    }
    temp_args[temp_argc++] = token;
    token = strtok(NULL, " ");
    if (temp_argc > ARG_MAX) {
      fprintf(stderr, "Error: cmd has more than 16 non-null arguments\n");
      return 1;
    }
  }

  // iterate through temp_args and check for >, >>,|
  for (int i = 0; i < temp_argc; i++) {
    if ((strcmp(temp_args[i], ">") == 0) || strcmp(temp_args[i], ">>") == 0) {
      rd_flag = 1;
      // if > or >> is the first command
      if (i == 0) {
        fprintf(stderr, "Error: missing command\n");
        return 1;
      } else if (i == temp_argc - 1) {
        // if nothing is after > or >>
        fprintf(stderr, "Error: no output file\n");
        return 1;
      }
    }
    if (strcmp(temp_args[i], "|") == 0) {
      // if > or >> come before |
      if (rd_flag == 1) {
        fprintf(stderr, "Error: mislocated output redirection\n");
        return 1;
      }
      if (i == 0) {
        // | is the first argument
        fprintf(stderr, "Error: missing command\n");
        return 1;
      } else if (i == temp_argc - 1) {
        // | is the last argument
        fprintf(stderr, "Error: missing command\n");
        return 1;
      }
    }
  }

  if (strcmp(temp_args[0], "cd") == 0) {
    if (temp_argc < 2) {
      fprintf(stderr, "Error: missing directory\n");
      return 1;
    }
    if (chdir(temp_args[1]) != 0) {
      fprintf(stderr, "Error: cannot cd into directory\n");
      fprintf(stderr, "+ completed '%s' [%d]\n", cmd, 1);
      return 1;
    }
  }

  if (strcmp(temp_args[0], "cat") == 0) {
    if (temp_argc < 2) {
      fprintf(stderr, "Error: missing file name\n");
      return 1;
    }
  }

  return 0;
}
// basic parsing by space
int parse_cmdline(char *cmd, struct cmdline *cmd_obj) {
  // save original command
  cmd_obj->original_cmd_holder = strdup(cmd);

  // parse cmdline arguments into array of arguments strings
  char *line = strtok(cmd, " ");
  int argc = 0;
  int redirect_flag = 0;
  cmd_obj->redirect_flag = 0;

  // check for & and remove it
  int last_char_position = strlen(cmd) - 1;
  if (cmd[last_char_position] == '&') {
    cmd[last_char_position] = '\0'; // remove '&'
  }

  while (line != NULL) {
    if (strstr(line, ">>") != NULL)
      redirect_flag = 2;
    else if (strchr(line, '>') != NULL)
      redirect_flag = 1;

    if (strchr(line, '>') != NULL || strstr(line, ">>") != NULL) { // found ">"
      // if > or >> is by itself
      if (strlen(line) == 1 || strlen(line) == 2) {
        line = strtok(NULL, " "); // grab the next token
        cmd_obj->filename = line; // copy the filename to obj
        line = strtok(NULL, " ");
        cmd_obj->redirect_flag = redirect_flag;
      } else { // if > or >> is connected with other arguments
        char *before_pointer = line; // used for args before > or >>
        char *after_pointer = NULL;  // used for args after > or >>

        if (redirect_flag == 1)
          after_pointer = strstr(line, ">");
        else if (redirect_flag == 2)
          after_pointer = strstr(line, ">>");

        if (after_pointer != NULL) {
          *after_pointer = '\0';
          after_pointer++;

          if (redirect_flag == 2) {
            *after_pointer = '\0';
            after_pointer++;
          }

          cmd_obj->args[argc++] = before_pointer;
          cmd_obj->filename = after_pointer;

          // if > or >> is not in the same argument as file
          // then next token is file name
          if (strlen(cmd_obj->filename) == 0) {
            line = strtok(NULL, " ");
            cmd_obj->filename = line;
            line = strtok(NULL, " ");
            cmd_obj->redirect_flag = redirect_flag;
          } else {
            line = strtok(NULL, " ");
            cmd_obj->redirect_flag = redirect_flag;
          }
        }
      }
    }
    cmd_obj->args[argc++] = line;
    line = strtok(NULL, " ");
  }

  // last element of args array is NULL
  cmd_obj->args[argc] = NULL;

  // parse cmd or first argument into struct obj
  cmd_obj->p_cmd = cmd_obj->args[0];

  // test file name
  if (redirect_flag > 0) { // if redirect flag is set
    FILE *fd_test;
    fd_test = fopen(cmd_obj->filename, "a"); // open file
    // check if file can be accessed
    if (access(cmd_obj->filename, W_OK) == -1) {
      fprintf(stderr, "Error: cannot open output file\n");
      return 1;
    }
    if ((fd_test != NULL)) { // if file already exist don't close
      fclose(fd_test);
    }
  }
  return 0;
}

int check_ampersand(char *cmd, struct cmdline *cmd_obj) {
  int last_char_position = strlen(cmd) - 1;
  int found_bg_flag = 0;

  for (int i = 0; i < (int)strlen(cmd); i++) {
    if (cmd[i] == '&')
      found_bg_flag = 1; // found &
    if (cmd[i] == '&' && i == last_char_position) {
      // if bg sign is found and at the end of cmdline then
      cmd_obj->background = 1; // set background flag
      cmd = strtok(cmd, "&");
    } else if (i != last_char_position && found_bg_flag == 1) {
      // if bg sign is found and not at the end then throw an error
      fprintf(stderr, "Error: mislocated background sign\n");
      return 1;
    }
  }
  cmd_obj->background = 0;
  return 0;
}

void file_redirect(struct cmdline cmd_obj) {
  int fd = 0;
  if (cmd_obj.redirect_flag == 0) {
    execvp(cmd_obj.p_cmd, cmd_obj.args);
    fprintf(stderr, "Error: command not found\n");
    exit(1);
  } else {
    if (cmd_obj.redirect_flag == 1) {
      fd = open(cmd_obj.filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    } else if (cmd_obj.redirect_flag == 2) {
      fd = open(cmd_obj.filename, O_WRONLY | O_CREAT | O_APPEND, 0644);
    }
    dup2(fd, STDOUT_FILENO);
    close(fd);
    execvp(cmd_obj.p_cmd, cmd_obj.args);
    fprintf(stderr, "Error: command not found\n");
    exit(1);
  }
}

//= parsing different command within a chained commands, set up for piping
struct node *pipe_parse(char *cmd) {
  struct node *head = (struct node *)malloc(sizeof(struct node));
  char *command = strtok(cmd, "|");
  char *cmd_array[CMDLINE_MAX];
  int num = 0;
  while (command != NULL) {
    cmd_array[num++] = command;
    command = strtok(NULL, "|");
  }
  cmd_array[num] = NULL;
  int i = 0;
  while (cmd_array[i] != NULL) {
    struct cmdline new_cmd; // struct that's going to be placed in the
    parse_cmdline(cmd_array[i], &new_cmd);
    appendLinkedlist(head, new_cmd);
    i++;
  }

  return head;
}

int execute_pipe_cmd(struct node *head, char *cmd) { // execute pipe commands
  // testing iterating linkedlist
  struct node *input_cmd = head;
  // int list_size= get_list_size(head);
  int pid, buffer[2]; // return_code[list_size];
  int prev_read = -1; // for next children to access the previous pipe's read fd
  int unused_fd = -1; // holds the prev_read of previous round so it can be
  // closed properly and free fd
  while (input_cmd != NULL) {
    if (input_cmd->next != NULL) { // last command
      if (pipe(buffer) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
      }
    }
    pid = fork();
    if (pid != 0) { // parent
      /* parent can't close read end of pipe so that the same pipe stay open
      for next child*/
      input_cmd->pid = pid; // store all child's pid
      close(buffer[1]);

    } else if (pid == 0 &&
               input_cmd->next == NULL) { // child that does last command
      dup2(prev_read, STDIN_FILENO);
      close(prev_read);
      file_redirect(input_cmd->data);
    } else if (pid == 0 && input_cmd == head) {
      close(buffer[0]);
      dup2(buffer[1], STDOUT_FILENO);
      close(buffer[1]);
      execvp(input_cmd->data.p_cmd, input_cmd->data.args);
    } else {
      close(buffer[0]);
      dup2(prev_read, STDIN_FILENO);
      close(prev_read);
      dup2(buffer[1], STDOUT_FILENO);
      close(buffer[1]);
      execvp(input_cmd->data.p_cmd, input_cmd->data.args);
    }
    if (prev_read != -1) { // if prev_read already has a value
      unused_fd = prev_read;
    }
    prev_read = buffer[0];
    close(unused_fd);
    input_cmd = input_cmd->next;
  }
  close(prev_read);

  // store exit status into linked list
  input_cmd = head;
  while (input_cmd != NULL) {
    waitpid(input_cmd->pid, &(input_cmd->status), 0);
    input_cmd = input_cmd->next;
  }

  // print the complete output
  fprintf(stderr, "+ completed '%s' ", cmd);
  input_cmd = head;
  while (input_cmd != NULL) {
    waitpid(input_cmd->pid, &(input_cmd->status), 0);
    printf("[%d]", input_cmd->status);
    input_cmd = input_cmd->next;
  }
  printf("\n");
  return 0;
}

// command that runs all kind of cmd such as bulleting, pipe command -> so the
// background child can execute all
int execute(char *cmd, char *original_cmd_holder, struct cmdline *cmd_obj) {

  int cmd_type = REGULAR_CMD;
  int status;
  if (!check_pipe(cmd)) {
    cmd_type = PIPING_CMD;
    struct node *head;
    head = pipe_parse(cmd);
    execute_pipe_cmd(head, original_cmd_holder);
		return 0;
  }

  /* parse cmdline */
  if (cmd_type != PIPING_CMD) {
    if(parse_cmdline(cmd, cmd_obj)!=0)
      return 0;
  }

  /* Builtin command */
  if (!strcmp(cmd_obj->p_cmd, "exit")) {
    fprintf(stderr, "Bye...\n");
    fprintf(stderr, "+ completed '%s' [%d]\n", original_cmd_holder, 0);
    return 1;
  }

  if (!strcmp(cmd_obj->p_cmd, "pwd")) {
    cmd_type = BULLETIN_CMD;
    status = pwd();
    fprintf(stderr, "+ completed '%s' [%d]\n", original_cmd_holder, status);
  }
  if (!strcmp(cmd_obj->p_cmd, "cd")) {
    cmd_type = BULLETIN_CMD;
    status = cd(cmd_obj->args);
    fprintf(stderr, "+ completed '%s' [%d]\n", original_cmd_holder, status);
  }

  /* regular command*/
  if (cmd_type == REGULAR_CMD) {
    int pid;
    pid = fork();

    // children execute cmd
    if (pid == 0) {
      file_redirect(*cmd_obj);
    }
    // parent wait
    else if (pid > 0) {
      int status;
      waitpid(pid, &status, 0);
      fprintf(stderr, "+ completed '%s' [%d]\n", original_cmd_holder, status);
      
    }
  }
  return 0;
}

int main(void) {
  char cmd[CMDLINE_MAX];
  struct cmdline cmd_obj;
  while (1) {
    char *nl;
    char original_cmd_holder[CMDLINE_MAX]; // save a copy of unprocessed cmd

    /* Print prompt */
    printf("sshell@ucd$ ");
    fflush(stdout);

    /* Get command line */
    if (fgets(cmd, CMDLINE_MAX, stdin) == NULL) {
      fprintf(stderr, "Error, input too long\n");
    }

    /* Print command line if stdin is not provided by terminal */
    if (!isatty(STDIN_FILENO)) {
      printf("%s", cmd);
      fflush(stdout);
    }

    /* Remove trailing newline from command line */
    nl = strchr(cmd, '\n');
    if (nl) {
      *nl = '\0';
    }

    // check when user input is blank
    if (cmd[0] == '\0') {
      continue;
    }

    // error management
    if (check_errors(cmd))
      continue;

    // make a copy of the command line
    strcpy(original_cmd_holder, cmd);

    // if ampersand was in incorrect place then igore the command
    if (check_ampersand(cmd, &cmd_obj))
      continue;

    // execute command
    if (execute(cmd, original_cmd_holder, &cmd_obj))
      break;
  }

  return EXIT_SUCCESS;
}