//
// Created by riseinokoe on 26.03.23.
//

#include "cmd.h"
#include <regex_lib.h>
#include <unistd.h>
#include <malloc.h>
#include <sys/wait.h>
#include <string.h>
#include <stdlib.h>

typedef struct m_pipe pipe_t;
typedef struct m_cmd cmd_t;
typedef struct regex_match reg_match_t;

void cmd_set(cmd_t * cmd, char **tokens, int len) {
    cmd->argc = len;
    cmd->name = strdup(tokens[0]);
    cmd->argv = (char**)malloc((len + 1) * sizeof(char*));
    for(int i = 0; i < len; ++i)
        cmd->argv[i] = strdup(tokens[i]);
    cmd->argv[len] = 0;
}

void cmd_free(cmd_t * cmd) {
    free(cmd->name);
    for(int i = 0; i < cmd->argc; ++i)
        free(cmd->argv[i]);
    free(cmd->argv);
}

void cmd_init(cmd_t* cmd) {
    cmd->set = &cmd_set;
    cmd->free = &cmd_free;
}

void pipe_set(pipe_t *pipe, cmd_t *cmds, int size) {
    pipe->commands = (cmd_t*) malloc(size * sizeof(cmd_t));
    pipe->size = size;
    for(int i = 0; i < size; ++i)
        pipe->commands[i] = cmds[i];
}

void pipe_free(pipe_t *pipe) {
    for(int i = 0; i < pipe->size; ++i)
        pipe->commands[i].free(&pipe->commands[i]);
    free(pipe->commands);
}

void pipe_init(pipe_t *pipe) {
    pipe->set = &pipe_set;
    pipe->free = &pipe_free;
}

void exec_cmd(cmd_t cmd, int prev_fd) {
    pid_t pid = fork();
    if(pid < 0) {
        fprintf(stderr, "[exec_cmd]: Couldn't start the fork\n");
        return;
    }
    else if(pid == 0) {
        if(prev_fd != -1) {
            close(prev_fd);
            dup2(prev_fd, STDIN_FILENO);
        }
        if(execvp(cmd.name, cmd.argv) < 0) {
            fprintf(stderr, "[exec_cmd]: Couldn't execute the command: %s\n", cmd.name);
            exit(EXIT_FAILURE);
        }
    }
    else {
        wait(NULL);
        return;
    }
}

int exec_pipe(cmd_t cmd1, cmd_t cmd2, int prev_fd) {
    int fd[2], new_fd = -1;
    if(pipe(fd) < 0) {
        fprintf(stderr, "[exec_pipe]: Couldn't initialize the pipe");
        return -1;
    }
    pid_t p1, p2;
    int status_1, status_2;
    p1 = fork();
    if(p1 == 0) {
        if(prev_fd != -1) {
            close(prev_fd);
            dup2(prev_fd, STDIN_FILENO);
        }
        close(fd[0]);
        dup2(fd[1], STDOUT_FILENO);
        close(fd[1]);
        if(execvp(cmd1.name, cmd1.argv) < 0) {
            fprintf(stderr, "[exec_pipe]: Couldn't execute the command: %s\n", cmd1.name);
            exit(EXIT_FAILURE);
        }
    }
    else if(p1 < 0) {
        fprintf(stderr, "[exec_pipe]: Couldn't start the fork\n");
        return -1;
    }
    else {
        p2 = fork();
        if(p2 == 0) {
            fprintf(stderr, "[exec_pipe]: Closing fd[1]...\n");
            close(fd[1]);
            fprintf(stderr, "[exec_pipe]: dup2 fd[0]...\n");
            dup2(fd[0], STDIN_FILENO);
            fprintf(stderr, "[exec_pipe]: Closing fd[0]...\n");
            close(fd[0]);
            new_fd = dup(STDOUT_FILENO);
            fprintf(stderr, "[exec_pipe]: new_fd = %d...\n", new_fd);
            fprintf(stderr, "[exec_pipe]: Closing new_fd...\n");
            close(new_fd);
            fprintf(stderr, "[exec_pipe]: Running execvp...\n");
            if(execvp(cmd2.name, cmd2.argv) < 0) {
                fprintf(stderr, "[exec_pipe]: Couldn't execute the command: %s\n", cmd2.name);
                exit(EXIT_FAILURE);
            }
            fprintf(stderr, "[exec_pipe]: Done...\n");
        }
        else if(p2 < 0) {
            fprintf(stderr, "[exec_pipe]: Couldn't start the fork\n");
            return -1;
        }
        else {
            fprintf(stderr, "[exec_pipe]: Waitin'\n");
            waitpid(p1, &status_1, 0);
            waitpid(p2, &status_2, 0);
            fprintf(stderr, "[exec_pipe]: Okay we done\n");
        }
    }
    return new_fd;
}

void exec(pipe_t cmds) {
    if(cmds.size == 1) {
        exec_cmd(cmds.commands[0], -1);
        return;
    }
    int tmp_fd = -1;
    printf("[exec]: cmds.size = %d", cmds.size);
    for(int i = 0; i + 1 < cmds.size; i += 2)
        exec_pipe(cmds.commands[i], cmds.commands[i + 1], tmp_fd);
    if(cmds.size & 1) {
        exec_cmd(cmds.commands[cmds.size - 1], tmp_fd);
    }
    else
        close(tmp_fd);
}

cmd_t parse_cmd(char *str) {
    cmd_t cmd;
    cmd_init(&cmd);
    reg_match_t tokens = match("([`]([^`]+)*[`])|([']([^']+)*['])|([\"]([^\"]+)*[\"])|([^[:space:]`'\"]+)", str, 10);
    if(tokens.n_matches < 0) {
        fprintf(stderr, "[parse_cmd]: No match, aborting...\n");
        exit(EXIT_FAILURE);
    }
    char **parsed = tokens.get_group(&tokens, 0);
    for(int i = 0; i < tokens.n_matches; ++i) {
        if(strcmp(parsed[i], "\0") == 0)
            continue;
        printf("[parse_cmd]: token: %s, matches: %d, groups: %d\n", parsed[i], tokens.n_matches, tokens.n_groups);
    }
    cmd.set(&cmd, parsed, tokens.n_matches);
    for(int i = 0; i < tokens.n_matches; ++i) {
        free(parsed[i]);
    }
    free(parsed);
    tokens.free(&tokens);
    return cmd;
}

pipe_t parse(char *str) {
    pipe_t new_pipe;
    pipe_init(&new_pipe);
    reg_match_t tokens = match("[|]?([^|]+)", str, 1);
    if(tokens.n_matches < 0) {
        fprintf(stderr, "[parse]: No match, aborting...");
        exit(EXIT_FAILURE);
    }
    char **parsed = tokens.get_group(&tokens, 1);
    cmd_t *cmds = (cmd_t*) malloc(tokens.n_matches * sizeof(cmd_t));
    for(int i = 0; i < tokens.n_matches; ++i)
        cmds[i] = parse_cmd(parsed[i]);
    new_pipe.set(&new_pipe, cmds, tokens.n_matches);
    for(int i = 0; i < tokens.n_matches; ++i)
        free(parsed[i]);
    free(parsed);
    free(cmds);
    tokens.free(&tokens);
    return new_pipe;
}