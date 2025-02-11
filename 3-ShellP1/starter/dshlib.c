#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "dshlib.h"

/*
 *  build_cmd_list
 *    cmd_line:     the command line from the user
 *    clist *:      pointer to clist structure to be populated
 *
 *  This function builds the command_list_t structure passed by the caller
 *  It does this by first splitting the cmd_line into commands by spltting
 *  the string based on any pipe characters '|'.  It then traverses each
 *  command.  For each command (a substring of cmd_line), it then parses
 *  that command by taking the first token as the executable name, and
 *  then the remaining tokens as the arguments.
 *
 *  NOTE your implementation should be able to handle properly removing
 *  leading and trailing spaces!
 *
 *  errors returned:
 *
 *    OK:                      No Error
 *    ERR_TOO_MANY_COMMANDS:   There is a limit of CMD_MAX (see dshlib.h)
 *                             commands.
 *    ERR_CMD_OR_ARGS_TOO_BIG: One of the commands provided by the user
 *                             was larger than allowed, either the
 *                             executable name, or the arg string.
 *
 *  Standard Library Functions You Might Want To Consider Using
 *      memset(), strcmp(), strcpy(), strtok(), strlen(), strchr()
 */
int build_cmd_list(char *cmd_line, command_list_t *clist)
{
    if (!cmd_line || strlen(cmd_line) == 0) {
        return WARN_NO_CMDS;
    }

    memset(clist, 0, sizeof(command_list_t)); // Initialize command list
    char *cmd_token;
    char *saveptr;
    int cmd_count = 0;

    // Tokenize by PIPE_CHAR '|'
    cmd_token = strtok_r(cmd_line, PIPE_STRING, &saveptr);
    while (cmd_token != NULL) {
        if (cmd_count >= CMD_MAX) {
            return ERR_TOO_MANY_COMMANDS;
        }
        
        // Trim leading and trailing spaces
        while (isspace((unsigned char)*cmd_token)) cmd_token++;
        char *end = cmd_token + strlen(cmd_token) - 1;
        while (end > cmd_token && isspace((unsigned char)*end)) {
            *end = '\0';
            end--;
        }
        
        // Extract executable name
        char *arg_token = strtok(cmd_token, " ");
        if (arg_token == NULL) {
            return WARN_NO_CMDS;
        }
        if (strlen(arg_token) >= EXE_MAX) {
            return ERR_CMD_OR_ARGS_TOO_BIG;
        }
        strcpy(clist->commands[cmd_count].exe, arg_token);
        
        // Extract arguments
        char args_buffer[ARG_MAX] = "";
        arg_token = strtok(NULL, " ");
        while (arg_token != NULL) {
            if (strlen(args_buffer) + strlen(arg_token) + 1 >= ARG_MAX) {
                return ERR_CMD_OR_ARGS_TOO_BIG;
            }
            strcat(args_buffer, arg_token);
            strcat(args_buffer, " ");
            arg_token = strtok(NULL, " ");
        }
        
        if (strlen(args_buffer) > 0) {
            args_buffer[strlen(args_buffer) - 1] = '\0'; // Remove trailing space
            strcpy(clist->commands[cmd_count].args, args_buffer);
        }
        
        cmd_count++;
        cmd_token = strtok_r(NULL, PIPE_STRING, &saveptr);
    }

    clist->num = cmd_count;
    return OK;
}
