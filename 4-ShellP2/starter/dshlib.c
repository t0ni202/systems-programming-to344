#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include "dshlib.h"
#include <errno.h>
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

// Allocate memory for cmd_buff structure
int alloc_cmd_buff(cmd_buff_t *cmd_buff) {
    if (!cmd_buff) return ERR_MEMORY;
    
    cmd_buff->argc = 0;
    cmd_buff->_cmd_buffer = NULL;
    for (int i = 0; i < CMD_ARGV_MAX; i++) {
        cmd_buff->argv[i] = NULL;
    }
    return OK;
}

// Free memory used by cmd_buff structure
int free_cmd_buff(cmd_buff_t *cmd_buff) {
    if (!cmd_buff) return ERR_MEMORY;
    
    if (cmd_buff->_cmd_buffer) {
        free(cmd_buff->_cmd_buffer);
        cmd_buff->_cmd_buffer = NULL;
    }
    
    cmd_buff->argc = 0;
    for (int i = 0; i < CMD_ARGV_MAX; i++) {
        cmd_buff->argv[i] = NULL;
    }
    return OK;
}

// Clear cmd_buff for reuse
int clear_cmd_buff(cmd_buff_t *cmd_buff) {
    if (!cmd_buff) return ERR_MEMORY;
    
    cmd_buff->argc = 0;
    if (cmd_buff->_cmd_buffer) {
        free(cmd_buff->_cmd_buffer);
        cmd_buff->_cmd_buffer = NULL;
    }
    for (int i = 0; i < CMD_ARGV_MAX; i++) {
        cmd_buff->argv[i] = NULL;
    }
    return OK;
}

// Parse command line into cmd_buff structure
int build_cmd_buff(char *cmd_line, cmd_buff_t *cmd_buff) {
    if (!cmd_line || !cmd_buff) return ERR_MEMORY;
    
    // Clear any existing command
    clear_cmd_buff(cmd_buff);
    
    // Make a copy of the command line
    cmd_buff->_cmd_buffer = strdup(cmd_line);
    if (!cmd_buff->_cmd_buffer) return ERR_MEMORY;
    
    char *token_start = cmd_buff->_cmd_buffer;
    bool in_quotes = false;
    int arg_count = 0;
    
    for (char *p = cmd_buff->_cmd_buffer; *p; p++) {
        if (*p == '"') {
            in_quotes = !in_quotes;
            // Remove the quote
            memmove(p, p + 1, strlen(p));
            p--;  // Adjust p since we removed a character
        } else if (isspace(*p) && !in_quotes) {
            if (p > token_start) {
                *p = '\0';
                if (arg_count >= CMD_ARGV_MAX - 1) {
                    return ERR_CMD_OR_ARGS_TOO_BIG;
                }
                cmd_buff->argv[arg_count++] = token_start;
                token_start = p + 1;
            } else {
                token_start = p + 1;
            }
        }
    }
    
    // Handle last token
    if (strlen(token_start) > 0) {
        if (arg_count >= CMD_ARGV_MAX - 1) {
            return ERR_CMD_OR_ARGS_TOO_BIG;
        }
        cmd_buff->argv[arg_count++] = token_start;
    }
    
    if (arg_count == 0) {
        return WARN_NO_CMDS;
    }
    
    cmd_buff->argc = arg_count;
    cmd_buff->argv[arg_count] = NULL;  // NULL terminate argv array
    
    return OK;
}

// global variable to store the last commands return code
static int last_return_code = 0;

// Main command execution loop
int exec_local_cmd_loop()
{
    char *cmd_buff;
    int rc = 0;
    cmd_buff_t cmd;

	// TODO IMPLEMENT MAIN LOOP
    if (alloc_cmd_buff(&cmd) != OK) {
        return ERR_MEMORY;
    }

    while(1) {
        printf("%s", SH_PROMPT);
        
        cmd_buff = (char *)malloc(SH_CMD_MAX);
        if (!cmd_buff) {
            free_cmd_buff(&cmd);
            return ERR_MEMORY;
        }

        if (fgets(cmd_buff, SH_CMD_MAX, stdin) == NULL) {
            printf("\n");
            free(cmd_buff);
            break;
        }

    // TODO IMPLEMENT parsing input to cmd_buff_t *cmd_buff
        // Remove trailing newline
        cmd_buff[strcspn(cmd_buff, "\n")] = '\0';

        // Skip empty lines
        if (strlen(cmd_buff) == 0) {
            printf(CMD_WARN_NO_CMD);
            free(cmd_buff);
            continue;
        }

        // Build command buffer from input
        rc = build_cmd_buff(cmd_buff, &cmd);
        free(cmd_buff);

        if (rc != OK) {
            if (rc == WARN_NO_CMDS) {
                printf(CMD_WARN_NO_CMD);
                continue;
            }
            break;
        }

    // TODO IMPLEMENT if built-in command, execute builtin logic for exit, cd (extra credit: dragon)
    // the cd command should chdir to the provided directory; if no directory is provided, do nothing
        // Check for exit command
        if (strcmp(cmd.argv[0], EXIT_CMD) == 0) {
            rc = OK_EXIT;
            break;
        }

        // Handle cd built-in command
        if (strcmp(cmd.argv[0], "cd") == 0) {
            if (cmd.argc > 1) {
					if(chdir(cmd.argv[1]) != 0){
						perror("cd");
					}

             }   
            clear_cmd_buff(&cmd);
            continue;
        }

		  // handle rc built in command
		  if(strcmp(cmd.argv[0], "rc") == 0){
		  		printf("%d\n", last_return_code);
				clear_cmd_buff(&cmd);
				continue;
			}

    // TODO IMPLEMENT if not built-in command, fork/exec as an external command
    // for example, if the user input is "ls -l", you would fork/exec the command "ls" with the arg "-l"
        // Fork/exec for external commands
        pid_t pid = fork();
        
        if (pid < 0) {
            perror("fork");
            clear_cmd_buff(&cmd);
            continue;
        }
        
        if (pid == 0) {  // Child process
            execvp(cmd.argv[0], cmd.argv);
            // If execvp returns, there was an error
				int exec_errno = errno;
				switch(exec_errno){
					case ENOENT:
						fprintf(stderr, "Command not found in PATH\n");
						break;
					case EACCES:
						fprintf(stderr, "Permission denied\n");
						break;
					case ENOMEM:
						fprintf(stderr, "Out of memory\n");
						break;
					case E2BIG:
						fprintf(stderr, "Argument list too long\n");
						break;
					default:
						fprintf(stderr, "Error executing command: %s\n", strerror(exec_errno));
					}
					exit(exec_errno); // return errno to the parent
            //perror("execvp");
            //exit(ERR_EXEC_CMD);
        } else {  // Parent process
            int status;
            waitpid(pid, &status, 0);
            /*if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
                printf("error executing command\n");
            }*/
				if (WIFEXITED(status)){
					last_return_code = WEXITSTATUS(status);
				} else {
					last_return_code = -1; // abnormal termination
				}
        }

        clear_cmd_buff(&cmd);
    }

    free_cmd_buff(&cmd);
    return rc;
}
