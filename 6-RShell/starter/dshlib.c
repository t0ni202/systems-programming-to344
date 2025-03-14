#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>

#include "dshlib.h"



#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>

#include "dshlib.h"


/*
 * alloc_cmd_buff - allocates memory for the command buffer
 * @cmd_buff: the command buffer structure to allocate memory for
 *
 * Returns OK on success, ERR_MEMORY on failure
 */
int alloc_cmd_buff(cmd_buff_t *cmd_buff) {
    cmd_buff->_cmd_buffer = malloc(SH_CMD_MAX);
    if (cmd_buff->_cmd_buffer == NULL) {
        return ERR_MEMORY;
    }
    cmd_buff->_cmd_buffer[0] = '\0';  // Initialize to empty string
    cmd_buff->argc = 0;
    cmd_buff->input_file = NULL;
    cmd_buff->output_file = NULL;
    cmd_buff->append_mode = false;
    
    // Initialize all argv pointers to NULL
    for (int i = 0; i < CMD_ARGV_MAX; i++) {
        cmd_buff->argv[i] = NULL;
    }
    
    return OK;
}

/*
 * free_cmd_buff - frees memory allocated for the command buffer
 * @cmd_buff: the command buffer structure to free
 *
 * Returns OK
 */
int free_cmd_buff(cmd_buff_t *cmd_buff) {
    if (cmd_buff->_cmd_buffer) {
        free(cmd_buff->_cmd_buffer);
        cmd_buff->_cmd_buffer = NULL;
    }
    
    cmd_buff->input_file = NULL;   // These point within _cmd_buffer, don't free separately
    cmd_buff->output_file = NULL;  // These point within _cmd_buffer, don't free separately
    cmd_buff->argc = 0;
    
    return OK;
}

/*
 * clear_cmd_buff - clears/resets the command buffer for reuse
 * @cmd_buff: the command buffer structure to clear
 *
 * Returns OK
 */
int clear_cmd_buff(cmd_buff_t *cmd_buff) {
    if (cmd_buff->_cmd_buffer) {
        cmd_buff->_cmd_buffer[0] = '\0';
    }
    
    cmd_buff->argc = 0;
    cmd_buff->input_file = NULL;
    cmd_buff->output_file = NULL;
    cmd_buff->append_mode = false;
    
    for (int i = 0; i < CMD_ARGV_MAX; i++) {
        cmd_buff->argv[i] = NULL;
    }
    
    return OK;
}

/*
 * build_cmd_buff - parses a command line string into the command buffer structure
 * @cmd_line: string containing the command to parse
 * @cmd_buff: the command buffer structure to populate
 *
 * Returns OK on success, error code on failure
 */
int build_cmd_buff(char *cmd_line, cmd_buff_t *cmd_buff) {
    char *token;
    char *saveptr;
    int argc = 0;
    
    if (cmd_line == NULL || cmd_buff == NULL || cmd_buff->_cmd_buffer == NULL) {
        return ERR_MEMORY;
    }
    
    // Copy the command line into our buffer
    strncpy(cmd_buff->_cmd_buffer, cmd_line, SH_CMD_MAX - 1);
    cmd_buff->_cmd_buffer[SH_CMD_MAX - 1] = '\0';
    
    // Clear any existing argc/argv data
    cmd_buff->argc = 0;
    for (int i = 0; i < CMD_ARGV_MAX; i++) {
        cmd_buff->argv[i] = NULL;
    }
    
    // Start tokenizing
    token = strtok_r(cmd_buff->_cmd_buffer, " \t\n", &saveptr);
    
    while (token != NULL && argc < CMD_ARGV_MAX - 1) {
        // Check for input redirection
        if (strcmp(token, "<") == 0) {
            token = strtok_r(NULL, " \t\n", &saveptr);
            if (token != NULL) {
                cmd_buff->input_file = token;
            }
        }
        // Check for output redirection
        else if (strcmp(token, ">") == 0) {
            token = strtok_r(NULL, " \t\n", &saveptr);
            if (token != NULL) {
                cmd_buff->output_file = token;
                cmd_buff->append_mode = false;
            }
        }
        // Check for append redirection
        else if (strcmp(token, ">>") == 0) {
            token = strtok_r(NULL, " \t\n", &saveptr);
            if (token != NULL) {
                cmd_buff->output_file = token;
                cmd_buff->append_mode = true;
            }
        }
        else {
            // Regular argument
            cmd_buff->argv[argc++] = token;
        }
        
        token = strtok_r(NULL, " \t\n", &saveptr);
    }
    
    // Set the argument count and NULL-terminate the argv array
    cmd_buff->argc = argc;
    cmd_buff->argv[argc] = NULL;
    
    return (argc > 0) ? OK : WARN_NO_CMDS;
}

/*
 * close_cmd_buff - closes/finalizes a command buffer
 * @cmd_buff: the command buffer to close
 *
 * Returns OK
 */
int close_cmd_buff(cmd_buff_t *cmd_buff) {
    return free_cmd_buff(cmd_buff);
}

/*
 * build_cmd_list - builds a list of piped commands from a command line
 * @cmd_line: the command line string to parse
 * @clist: the command list structure to populate
 *
 * Returns OK on success, error code on failure
 */
int build_cmd_list(char *cmd_line, command_list_t *clist) {
    char *token;
    char *saveptr;
    char *cmd_copy;
    int num_cmds = 0;
    int rc;
    
    if (cmd_line == NULL || clist == NULL) {
        return ERR_MEMORY;
    }
    
    // Make a copy of the command line for tokenizing
    cmd_copy = strdup(cmd_line);
    if (cmd_copy == NULL) {
        return ERR_MEMORY;
    }
    
    // Initialize command count
    clist->num = 0;
    
    // First count the number of commands (pipe delimited)
    char *cmd_ptr = cmd_copy;
    while (*cmd_ptr) {
        if (*cmd_ptr == PIPE_CHAR) {
            num_cmds++;
        }
        cmd_ptr++;
    }
    num_cmds++; // Add one for the last command
    
    if (num_cmds > CMD_MAX) {
        free(cmd_copy);
        printf(CMD_ERR_PIPE_LIMIT, CMD_MAX);
        return ERR_TOO_MANY_COMMANDS;
    }
    
    // Reset our copy for tokenizing
    free(cmd_copy);
    cmd_copy = strdup(cmd_line);
    if (cmd_copy == NULL) {
        return ERR_MEMORY;
    }
    
    // Tokenize by pipes and build each command
    token = strtok_r(cmd_copy, PIPE_STRING, &saveptr);
    num_cmds = 0;
    
    while (token != NULL && num_cmds < CMD_MAX) {
        // Allocate memory for this command's buffer
        rc = alloc_cmd_buff(&clist->commands[num_cmds]);
        if (rc != OK) {
            free(cmd_copy);
            return rc;
        }
        
        // Build the command buffer for this command
        rc = build_cmd_buff(token, &clist->commands[num_cmds]);
        if (rc == WARN_NO_CMDS) {
            // Skip empty commands
            free_cmd_buff(&clist->commands[num_cmds]);
        } else if (rc != OK) {
            // Handle errors
            free(cmd_copy);
            return rc;
        } else {
            // Valid command
            num_cmds++;
        }
        
        token = strtok_r(NULL, PIPE_STRING, &saveptr);
    }
    
    clist->num = num_cmds;
    free(cmd_copy);
    
    if (num_cmds == 0) {
        return WARN_NO_CMDS;
    }
    
    return OK;
}

/*
 * free_cmd_list - frees memory allocated for a command list
 * @cmd_lst: the command list to free
 *
 * Returns OK
 */
int free_cmd_list(command_list_t *cmd_lst) {
    if (cmd_lst == NULL) {
        return OK;
    }
    
    for (int i = 0; i < cmd_lst->num; i++) {
        free_cmd_buff(&cmd_lst->commands[i]);
    }
    
    cmd_lst->num = 0;
    return OK;
}

/*
 * match_command - checks if a command string matches a built-in command
 * @input: the command string to check
 *
 * Returns the matching built-in command type, or BI_NOT_BI if not a built-in
 */
Built_In_Cmds match_command(const char *input) {
    if (strcmp(input, "exit") == 0)
        return BI_CMD_EXIT;
    if (strcmp(input, "dragon") == 0)
        return BI_CMD_DRAGON;
    if (strcmp(input, "cd") == 0)
        return BI_CMD_CD;
    if (strcmp(input, "rc") == 0)
        return BI_CMD_RC;
    return BI_NOT_BI;
}

/*
 * exec_built_in_cmd - executes a built-in command
 * @cmd: the command buffer containing the command to execute
 *
 * Returns the result of the built-in command execution
 */
Built_In_Cmds exec_built_in_cmd(cmd_buff_t *cmd) {
    Built_In_Cmds ctype = BI_NOT_BI;
    
    if (cmd == NULL || cmd->argc == 0) {
        return BI_NOT_BI;
    }
    
    ctype = match_command(cmd->argv[0]);
    
    switch (ctype) {
    case BI_CMD_DRAGON:
        printf("          ______________\n");
        printf("         /             /|\n");
        printf("        /             / |\n");
        printf("       /____________ /  |\n");
        printf("      | ___________ |   |\n");
        printf("      ||           ||   |\n");
        printf("      ||           ||   |\n");
        printf("      ||           ||   |\n");
        printf("      ||___________||   |\n");
        printf("      |   _______   |  /\n");
        printf("      |  |       |  | /\n");
        printf("      |__|_______|__|/\n");
        return BI_EXECUTED;
    case BI_CMD_EXIT:
        return BI_CMD_EXIT;
    case BI_CMD_CD:
        if (cmd->argc < 2) {
            // No argument, change to HOME directory
            chdir(getenv("HOME"));
        } else {
            if (chdir(cmd->argv[1]) != 0) {
                perror("cd");
            }
        }
        return BI_EXECUTED;
    case BI_CMD_RC:
        printf("%d\n", EXIT_SC);
        return BI_EXECUTED;
    default:
        return BI_NOT_BI;
    }
}

/*
 * execute_pipeline - executes a pipeline of commands
 * @clist: the command list containing the pipeline
 *
 * Returns the exit code of the last command in the pipeline
 */
int execute_pipeline(command_list_t *clist) {
    int pipes[CMD_MAX-1][2];  // Array of pipes
    pid_t pids[CMD_MAX];      // Array to store process IDs
    int pids_st[CMD_MAX];     // Array to store process statuses
    Built_In_Cmds bi_cmd;
    
    // Create all necessary pipes
    for (int i = 0; i < clist->num - 1; i++) {
        if (pipe(pipes[i]) == -1) {
            perror("pipe");
            return ERR_EXEC_CMD;
        }
    }
    
    for (int i = 0; i < clist->num; i++) {
        // Check if it's a built-in command (only for first command in pipeline)
        if (i == 0) {
            bi_cmd = exec_built_in_cmd(&clist->commands[i]);
            if (bi_cmd == BI_EXECUTED) {
                // If it's a built-in command and was executed successfully, no need to fork
                // Close all pipes
                for (int j = 0; j < clist->num - 1; j++) {
                    close(pipes[j][0]);
                    close(pipes[j][1]);
                }
                return OK;
            } else if (bi_cmd == BI_CMD_EXIT) {
                // If it's the exit command, return OK_EXIT
                for (int j = 0; j < clist->num - 1; j++) {
                    close(pipes[j][0]);
                    close(pipes[j][1]);
                }
                return OK_EXIT;
            }
        }
        
        // Fork a child process for each command
        pids[i] = fork();
        
        if (pids[i] < 0) {
            // Fork error
            perror("fork");
            return ERR_EXEC_CMD;
        } else if (pids[i] == 0) {
            // Child process
            
            // Setup input redirection for first command
            if (i == 0 && clist->commands[i].input_file != NULL) {
                int fd = open(clist->commands[i].input_file, O_RDONLY);
                if (fd < 0) {
                    perror("open input file");
                    exit(ERR_EXEC_CMD);
                }
                dup2(fd, STDIN_FILENO);
                close(fd);
            }
            
            // Setup output redirection for last command
            if (i == clist->num - 1 && clist->commands[i].output_file != NULL) {
                int flags = O_WRONLY | O_CREAT;
                if (clist->commands[i].append_mode) {
                    flags |= O_APPEND;
                } else {
                    flags |= O_TRUNC;
                }
                
                int fd = open(clist->commands[i].output_file, flags, 0644);
                if (fd < 0) {
                    perror("open output file");
                    exit(ERR_EXEC_CMD);
                }
                dup2(fd, STDOUT_FILENO);
                close(fd);
            }
            
            // Setup pipe for input (if not the first command)
            if (i > 0) {
                dup2(pipes[i-1][0], STDIN_FILENO);
            }
            
            // Setup pipe for output (if not the last command)
            if (i < clist->num - 1) {
                dup2(pipes[i][1], STDOUT_FILENO);
            }
            
            // Close all pipe ends in child
            for (int j = 0; j < clist->num - 1; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }
            
            // Execute the command
            execvp(clist->commands[i].argv[0], clist->commands[i].argv);
            perror("execvp");
            exit(ERR_EXEC_CMD);
        }
    }
    
    // Parent process: close all pipe ends
    for (int i = 0; i < clist->num - 1; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }
    
    // Wait for all children
    for (int i = 0; i < clist->num; i++) {
        waitpid(pids[i], &pids_st[i], 0);
    }
    
    // Return the exit code of the last command
    return WEXITSTATUS(pids_st[clist->num - 1]);
}

/*
 * exec_cmd - executes a single command
 * @cmd: the command buffer containing the command to execute
 *
 * Returns the result of the command execution
 */
int exec_cmd(cmd_buff_t *cmd) {
    pid_t pid;
    int status;
    Built_In_Cmds bi_cmd;
    
    // Check if it's a built-in command
    bi_cmd = exec_built_in_cmd(cmd);
    if (bi_cmd == BI_EXECUTED) {
        return OK;
    } else if (bi_cmd == BI_CMD_EXIT) {
        return OK_EXIT;
    }
    
    // Not a built-in command, execute it with fork/exec
    pid = fork();
    
    if (pid < 0) {
        // Fork error
        perror("fork");
        return ERR_EXEC_CMD;
    } else if (pid == 0) {
        // Child process
        
        // Handle input redirection
        if (cmd->input_file != NULL) {
            int fd = open(cmd->input_file, O_RDONLY);
            if (fd < 0) {
                perror("open input file");
                exit(ERR_EXEC_CMD);
            }
            dup2(fd, STDIN_FILENO);
            close(fd);
        }
        
        // Handle output redirection
        if (cmd->output_file != NULL) {
            int flags = O_WRONLY | O_CREAT;
            if (cmd->append_mode) {
                flags |= O_APPEND;
            } else {
                flags |= O_TRUNC;
            }
            
            int fd = open(cmd->output_file, flags, 0644);
            if (fd < 0) {
                perror("open output file");
                exit(ERR_EXEC_CMD);
            }
            dup2(fd, STDOUT_FILENO);
            close(fd);
        }
        
        // Execute the command
        execvp(cmd->argv[0], cmd->argv);
        perror("execvp");
        exit(ERR_EXEC_CMD);
    } else {
        // Parent process
        waitpid(pid, &status, 0);
        return WEXITSTATUS(status);
    }
}








/**** 
 **** FOR REMOTE SHELL USE YOUR SOLUTION FROM SHELL PART 3 HERE
 **** THE MAIN FUNCTION CALLS THIS ONE AS ITS ENTRY POINT TO
 **** EXECUTE THE SHELL LOCALLY
 ****
 */

/*
 * Implement your exec_local_cmd_loop function by building a loop that prompts the 
 * user for input.  Use the SH_PROMPT constant from dshlib.h and then
 * use fgets to accept user input.
 * 
 *      while(1){
 *        printf("%s", SH_PROMPT);
 *        if (fgets(cmd_buff, ARG_MAX, stdin) == NULL){
 *           printf("\n");
 *           break;
 *        }
 *        //remove the trailing \n from cmd_buff
 *        cmd_buff[strcspn(cmd_buff,"\n")] = '\0';
 * 
 *        //IMPLEMENT THE REST OF THE REQUIREMENTS
 *      }
 * 
 *   Also, use the constants in the dshlib.h in this code.  
 *      SH_CMD_MAX              maximum buffer size for user input
 *      EXIT_CMD                constant that terminates the dsh program
 *      SH_PROMPT               the shell prompt
 *      OK                      the command was parsed properly
 *      WARN_NO_CMDS            the user command was empty
 *      ERR_TOO_MANY_COMMANDS   too many pipes used
 *      ERR_MEMORY              dynamic memory management failure
 * 
 *   errors returned
 *      OK                     No error
 *      ERR_MEMORY             Dynamic memory management failure
 *      WARN_NO_CMDS           No commands parsed
 *      ERR_TOO_MANY_COMMANDS  too many pipes used
 *   
 *   console messages
 *      CMD_WARN_NO_CMD        print on WARN_NO_CMDS
 *      CMD_ERR_PIPE_LIMIT     print on ERR_TOO_MANY_COMMANDS
 *      CMD_ERR_EXECUTE        print on execution failure of external command
 * 
 *  Standard Library Functions You Might Want To Consider Using (assignment 1+)
 *      malloc(), free(), strlen(), fgets(), strcspn(), printf()
 * 
 *  Standard Library Functions You Might Want To Consider Using (assignment 2+)
 *      fork(), execvp(), exit(), chdir()
 */
int exec_local_cmd_loop()
{
    char cmd_buff[SH_CMD_MAX];
    command_list_t cmd_list;
    int rc;
    
    while(1) {
        printf("%s", SH_PROMPT);
        
        if (fgets(cmd_buff, SH_CMD_MAX, stdin) == NULL) {
            printf("\n");
            break;
        }
        
        // Remove the trailing newline
        cmd_buff[strcspn(cmd_buff, "\n")] = '\0';
        
        // Skip empty commands
        if (strlen(cmd_buff) == 0) {
            continue;
        }
        
        // Build the command list (handles parsing and setting up for pipes)
        memset(&cmd_list, 0, sizeof(command_list_t));
        rc = build_cmd_list(cmd_buff, &cmd_list);
        
        if (rc == WARN_NO_CMDS) {
            printf(CMD_WARN_NO_CMD);
            continue;
        } else if (rc == ERR_TOO_MANY_COMMANDS) {
            printf(CMD_ERR_PIPE_LIMIT, CMD_MAX);
            continue;
        } else if (rc != OK) {
            // Other errors
            return rc;
        }
        
        // Execute the command pipeline
        rc = execute_pipeline(&cmd_list);
        
        // Free resources used by the command list
        free_cmd_list(&cmd_list);
        
        // Check if we need to exit
        if (rc == OK_EXIT) {
            return OK;
        }
    }    
    return OK;
}

