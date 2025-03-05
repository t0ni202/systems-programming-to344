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
 * Global arrays to hold redirection info for each command (by index)
 */
static char *redir_in[CMD_MAX] = { NULL };
static char *redir_out[CMD_MAX] = { NULL };
static int   redir_append[CMD_MAX] = { 0 };  // 0 = overwrite; 1 = append

/*
 * trim_whitespace:
 * Removes leading and trailing whitespace from a string.
 */
static char *trim_whitespace(char *str) {
    while(isspace((unsigned char)*str)) str++;
    if(*str == '\0')
        return str;
    char *end = str + strlen(str) - 1;
    while(end > str && isspace((unsigned char)*end))
        end--;
    *(end+1) = '\0';
    return str;
}

/*
 * parse_redirection:
 * Scans through cmd->argv for "<", ">", or ">>". When found,
 * saves the subsequent token into the global arrays (using command index idx)
 * and removes the redirection tokens from the argv array.
 */
static int parse_redirection(cmd_buff_t *cmd, int idx) {
    int new_index = 0, i = 0;
    while (cmd->argv[i] != NULL) {
        if (strcmp(cmd->argv[i], "<") == 0) {
            if (cmd->argv[i+1] == NULL) {
                fprintf(stderr, "error: no input file specified\n");
                return ERR_CMD_ARGS_BAD;
            }
            if (redir_in[idx]) { free(redir_in[idx]); redir_in[idx] = NULL; }
            redir_in[idx] = strdup(cmd->argv[i+1]);
            i += 2;  // skip operator and filename
        } else if (strcmp(cmd->argv[i], ">") == 0) {
            if (cmd->argv[i+1] == NULL) {
                fprintf(stderr, "error: no output file specified\n");
                return ERR_CMD_ARGS_BAD;
            }
            if (redir_out[idx]) { free(redir_out[idx]); redir_out[idx] = NULL; }
            redir_out[idx] = strdup(cmd->argv[i+1]);
            redir_append[idx] = 0;  // overwrite mode
            i += 2;
        } else if (strcmp(cmd->argv[i], ">>") == 0) {
            if (cmd->argv[i+1] == NULL) {
                fprintf(stderr, "error: no output file specified for append\n");
                return ERR_CMD_ARGS_BAD;
            }
            if (redir_out[idx]) { free(redir_out[idx]); redir_out[idx] = NULL; }
            redir_out[idx] = strdup(cmd->argv[i+1]);
            redir_append[idx] = 1;  // append mode
            i += 2;
        } else {
            cmd->argv[new_index++] = cmd->argv[i++];
        }
    }
    for (int j = new_index; j < CMD_ARGV_MAX; j++) {
        cmd->argv[j] = NULL;
    }
    cmd->argc = new_index;
    return OK;
}

/*
 * alloc_cmd_buff:
 * Initializes a cmd_buff_t structure.
 */
int alloc_cmd_buff(cmd_buff_t *cmd_buff) {
    if (!cmd_buff)
        return ERR_MEMORY;
    cmd_buff->argc = 0;
    cmd_buff->_cmd_buffer = NULL;
    return OK;
}

/*
 * clear_cmd_buff:
 * Frees any allocated tokens in cmd_buff->argv and the _cmd_buffer.
 */
int clear_cmd_buff(cmd_buff_t *cmd_buff) {
    if (!cmd_buff)
        return ERR_MEMORY;
    for (int i = 0; i < CMD_ARGV_MAX; i++) {
        if (cmd_buff->argv[i]) {
            free(cmd_buff->argv[i]);
            cmd_buff->argv[i] = NULL;
        }
    }
    if (cmd_buff->_cmd_buffer) {
        free(cmd_buff->_cmd_buffer);
        cmd_buff->_cmd_buffer = NULL;
    }
    cmd_buff->argc = 0;
    return OK;
}

int free_cmd_buff(cmd_buff_t *cmd_buff) {
    return clear_cmd_buff(cmd_buff);
}

int close_cmd_buff(cmd_buff_t *cmd_buff) {
    return clear_cmd_buff(cmd_buff);
}

/*
 * build_cmd_buff:
 * Tokenizes a single command (with no pipe characters) from cmd_line
 * into a cmd_buff_t. Uses strtok with whitespace.
 */
int build_cmd_buff(char *cmd_line, cmd_buff_t *cmd_buff) {
    if (!cmd_line || !cmd_buff)
        return ERR_MEMORY;
    int rc = alloc_cmd_buff(cmd_buff);
    if (rc != OK)
        return rc;
    cmd_buff->_cmd_buffer = strdup(cmd_line);
    if (!cmd_buff->_cmd_buffer)
        return ERR_MEMORY;
    int idx = 0;
    char *token = strtok(cmd_buff->_cmd_buffer, " \t");
    while (token != NULL && idx < CMD_ARGV_MAX - 1) {
        cmd_buff->argv[idx++] = strdup(token);
        token = strtok(NULL, " \t");
    }
    cmd_buff->argv[idx] = NULL;
    cmd_buff->argc = idx;
    return OK;
}

/*
 * build_cmd_list:
 * Splits a full command line (which may contain pipes) into a command_list_t.
 * Each segment is trimmed and passed to build_cmd_buff.
 * Then, parse_redirection() is called to extract any redirection tokens.
 */
int build_cmd_list(char *cmd_line, command_list_t *clist) {
    if (!cmd_line || !clist)
        return ERR_MEMORY;

    clist->num = 0;
    // Clear any previous redirection globals.
    for (int i = 0; i < CMD_MAX; i++) {
        if (redir_in[i]) { free(redir_in[i]); redir_in[i] = NULL; }
        if (redir_out[i]) { free(redir_out[i]); redir_out[i] = NULL; }
        redir_append[i] = 0;
    }

    // Split the input line by the pipe symbol.
    char *token = strtok(cmd_line, PIPE_STRING);
    while (token != NULL && clist->num < CMD_MAX) {
        // Trim leading and trailing whitespace.
        char *trimmed = trim_whitespace(token);
        if (strlen(trimmed) == 0) {
            token = strtok(NULL, PIPE_STRING);
            continue;
        }
        int rc = build_cmd_buff(trimmed, &clist->commands[clist->num]);
        if (rc != OK)
            return rc;

        // Process redirection tokens and store redirection info.
        rc = parse_redirection(&clist->commands[clist->num], clist->num);
        if (rc != OK)
            return rc;

        clist->num++;
        token = strtok(NULL, PIPE_STRING);
    }
    
    if (clist->num == 0) {
        fprintf(stderr, CMD_WARN_NO_CMD);
        return WARN_NO_CMDS;
    }
    
    // Optional: Enforce an upper limit (though the loop condition usually prevents overflow)
    if (clist->num > CMD_MAX) {
        fprintf(stderr, CMD_ERR_PIPE_LIMIT, CMD_MAX);
        return ERR_TOO_MANY_COMMANDS;
    }
    
    return OK;
}

int free_cmd_list(command_list_t *cmd_lst) {
    if (!cmd_lst)
        return ERR_MEMORY;
    for (int i = 0; i < cmd_lst->num; i++) {
        free_cmd_buff(&cmd_lst->commands[i]);
        if (redir_in[i]) { free(redir_in[i]); redir_in[i] = NULL; }
        if (redir_out[i]) { free(redir_out[i]); redir_out[i] = NULL; }
        redir_append[i] = 0;
    }
    return OK;
}

/*
 * match_command:
 * Checks if the first token of a command matches a built-in command.
 */
Built_In_Cmds match_command(const char *input) {
    if (strcmp(input, EXIT_CMD) == 0)
        return BI_CMD_EXIT;
    else if (strcmp(input, "dragon") == 0)
        return BI_CMD_DRAGON;
    else if (strcmp(input, "cd") == 0)
        return BI_CMD_CD;
    else
        return BI_NOT_BI;
}

/*
 * exec_built_in_cmd:
 * Executes a built-in command without forking.
 */
Built_In_Cmds exec_built_in_cmd(cmd_buff_t *cmd) {
    Built_In_Cmds type = match_command(cmd->argv[0]);
    if (type == BI_CMD_EXIT) {
        exit(EXIT_SC);
    } else if (type == BI_CMD_CD) {
        if (cmd->argc < 2) {
            fprintf(stderr, "cd: missing operand\n");
        } else {
            if (chdir(cmd->argv[1]) != 0)
                perror("cd");
        }
        return BI_EXECUTED;
    } else if (type == BI_CMD_DRAGON) {
        printf("Roar! Dragon command executed!\n");
        return BI_EXECUTED;
    }
    return BI_NOT_BI;
}

/*
 * exec_cmd:
 * Executes a single (non-pipelined) command. Before calling execvp,
 * sets up I/O redirection based on redir_in[0], redir_out[0], and redir_append[0].
 */
int exec_cmd(cmd_buff_t *cmd) {
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return ERR_EXEC_CMD;
    } else if (pid == 0) {  // Child
        int fd;
        if (redir_in[0] != NULL) {
            fd = open(redir_in[0], O_RDONLY);
            if (fd < 0) {
                perror("open input redirection");
                exit(ERR_EXEC_CMD);
            }
            dup2(fd, STDIN_FILENO);
            close(fd);
        }
        if (redir_out[0] != NULL) {
            if (redir_append[0])
                fd = open(redir_out[0], O_WRONLY | O_CREAT | O_APPEND, 0644);
            else
                fd = open(redir_out[0], O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd < 0) {
                perror("open output redirection");
                exit(ERR_EXEC_CMD);
            }
            dup2(fd, STDOUT_FILENO);
            close(fd);
        }
        execvp(cmd->argv[0], cmd->argv);
        perror("execvp");
        exit(ERR_EXEC_CMD);
    } else {  // Parent
        int status;
        waitpid(pid, &status, 0);
        return OK;
    }
}

/*
 * execute_pipeline:
 * Executes the commands in the command list as a pipeline.
 * This version allocates an array of pipes: one pipe per gap between commands,
 * then forks each command. For command i:
 *   - If i > 0, its STDIN is set to the read end of the (i-1)th pipe.
 *   - If i < num_commands - 1, its STDOUT is set to the write end of the i-th pipe.
 * In the child process, all pipe file descriptors are closed after duplication.
 * The parent process closes all pipe FDs and waits for all children.
 */
int execute_pipeline(command_list_t *clist) {
    int num_commands = clist->num;
    if (num_commands < 1)
        return WARN_NO_CMDS;
    
    // Create an array of pipes. For N commands, we need N-1 pipes.
    int pipes[num_commands - 1][2];
    pid_t child_pids[CMD_MAX];

    // Create pipes.
    for (int i = 0; i < num_commands - 1; i++) {
        if (pipe(pipes[i]) < 0) {
            perror("pipe creation failed");
            return ERR_MEMORY;
        }
    }
    
    // Fork each command.
    for (int i = 0; i < num_commands; i++) {
        child_pids[i] = fork();
        if (child_pids[i] < 0) {
            perror("fork failed");
            // Clean up any pipes.
            for (int j = 0; j < num_commands - 1; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }
            return ERR_MEMORY;
        }
        if (child_pids[i] == 0) {  // Child process
            // If not the first command, set STDIN to the read end of the previous pipe.
            if (i > 0) {
                if (dup2(pipes[i - 1][0], STDIN_FILENO) < 0) {
                    perror("dup2 stdin failed");
                    exit(ERR_EXEC_CMD);
                }
            }
            // If not the last command, set STDOUT to the write end of the current pipe.
            if (i < num_commands - 1) {
                if (dup2(pipes[i][1], STDOUT_FILENO) < 0) {
                    perror("dup2 stdout failed");
                    exit(ERR_EXEC_CMD);
                }
            }
            // Close all pipe file descriptors in the child.
            for (int j = 0; j < num_commands - 1; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }
            // Execute the command.
            execvp(clist->commands[i].argv[0], clist->commands[i].argv);
            perror("execvp failed");
            exit(ERR_EXEC_CMD);
        }
    }
    
    // Parent process: close all pipe FDs.
    for (int i = 0; i < num_commands - 1; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }
    
    // Wait for all child processes.
    int last_status = OK;
    for (int i = 0; i < num_commands; i++) {
        int status;
        waitpid(child_pids[i], &status, 0);
        if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
            last_status = ERR_EXEC_CMD;
        }
    }
    return last_status;
}

/*
 * exec_local_cmd_loop:
 * Main shell loop. Prompts the user, reads input, checks for EXIT_CMD,
 * parses the command line into a command list (handling pipes and redirection),
 * and then either executes a built-in command or runs the pipeline.
 */
int exec_local_cmd_loop() {
    char cmd_line[SH_CMD_MAX];
    command_list_t clist;
    int rc;
    
    while (1) {
        printf("%s", SH_PROMPT);
        if (fgets(cmd_line, sizeof(cmd_line), stdin) == NULL) {
            printf("\n");
            break;
        }
        cmd_line[strcspn(cmd_line, "\n")] = '\0';
        if (strcmp(cmd_line, EXIT_CMD) == 0) {
            printf("exiting...\n");
            break;
        }
        char *line_copy = strdup(cmd_line);
        if (!line_copy) {
            perror("strdup");
            continue;
        }
        rc = build_cmd_list(line_copy, &clist);
        free(line_copy);
        if (rc != OK)
            continue;
        if (clist.num < 1) {
            fprintf(stderr, CMD_WARN_NO_CMD);
            free_cmd_list(&clist);
            continue;
        }
        Built_In_Cmds bi = match_command(clist.commands[0].argv[0]);
        if (bi != BI_NOT_BI) {
            exec_built_in_cmd(&clist.commands[0]);
            free_cmd_list(&clist);
            continue;
        }
        if (clist.num == 1)
            rc = exec_cmd(&clist.commands[0]);
        else
            rc = execute_pipeline(&clist);
        free_cmd_list(&clist);
    }
    return OK;
}
