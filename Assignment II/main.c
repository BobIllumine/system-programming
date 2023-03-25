#include <stdio.h>
#include <malloc.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <string.h>
#include <regex.h>
#include "heap_help/heap_help.h"
#include "regex_lib/regex_lib.h"

#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))

typedef struct m_cmd {
    char *name;
    char **argv;
    int argc;
    void (*set)(struct m_cmd*, char**, int);
    void (*free)(struct m_cmd*);
} cmd_t;

typedef struct m_pipe {
    cmd_t *commands;
    int size;
    void (*set)(struct m_pipe*, cmd_t*, int);
    void (*free)(struct m_pipe*);
} pipe_t;

typedef struct regex_match reg_match_t;

void cmd_set(cmd_t * command, char **tokens, int len) {
    command->argc = len;
    command->name = strdup(tokens[0]);
    command->argv = (char**)malloc(len * sizeof(char*));
    for(int i = 0; i < len; ++i)
        command->argv[i] = strdup(tokens[i]);
}

void cmd_free(cmd_t * command) {
    free(command->name);
    for(int i = 0; i < command->argc; ++i)
        free(command->argv[i]);
    free(command->argv);
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

void exec_cmd(cmd_t command) {
    pid_t pid = fork();
    if(pid == -1) {
        fprintf(stderr, "[exec_cmd]: Couldn't start the fork\n");
        return;
    }
    else if(pid == 0) {
        if(execvp(command.name, command.argv) < 0) {
            fprintf(stderr, "[exec_cmd]: Couldn't execute the command: %s\n", command.name);
            exit(EXIT_FAILURE);
        }
    }
    else {
        wait(NULL);
        return;
    }
}

void exec(pipe_t cmds) {
    if(cmds.size == 1) {
        exec_cmd(cmds.commands[0]);
        return;
    }
    // 0 is p2, 1 is p1
    int fd[2];
    if(pipe(fd) < 0) {
        fprintf(stderr, "[exec]: Couldn't initialize the pipe\n");
        return;
    }
    pid_t p_start, p_tmp, p_end;
    p_start = fork();
    if(p_start < 0) {
        fprintf(stderr, "[exec]: Couldn't start the fork\n");
        return;
    }
    if(p_start == 0) {
        close(fd[0]);
        dup2(fd[1], STDOUT_FILENO);
        close(fd[1]);
        if(execvp(cmds.commands[0].name, cmds.commands[0].argv)) {
            fprintf(stderr, "[exec]: Couldn't execute the command: %s\n", cmds.commands[0].name);
            exit(EXIT_FAILURE);
        }
    }
    else {
        wait(NULL);
    }
    for(int i = 1; i < cmds.size - 1; ) {
        p_tmp = fork();
        if(p_tmp < 0) {
            fprintf(stderr, "[exec]: Couldn't start the fork\n");
            return;
        }
        if(p_tmp == 0) {
            dup2(fd[0], STDIN_FILENO);
            close(fd[0]);
            dup2(fd[1], STDOUT_FILENO);
            close(fd[1]);
            if(execvp(cmds.commands[i].name, cmds.commands[i].argv)) {
                fprintf(stderr, "[exec]: Couldn't execute the command: %s\n", cmds.commands[i].name);
                exit(EXIT_FAILURE);
            }
            ++i;
        }
        else {
            wait(NULL);
        }
    }
    p_end = fork();
    if(p_end < 0) {
        fprintf(stderr, "[exec]: Couldn't start the fork\n");
        return;
    }
    if(p_end == 0) {
        close(fd[1]);
        dup2(fd[0], STDIN_FILENO);
        close(fd[0]);
        if(execvp(cmds.commands[cmds.size - 1].name, cmds.commands[cmds.size - 1].argv)) {
            fprintf(stderr, "[exec]: Couldn't execute the command: %s\n", cmds.commands[cmds.size - 1].name);
            exit(EXIT_FAILURE);
        }
    }
    else {
        wait(NULL);
    }

}

cmd_t parse_command(char *str) {
    cmd_t command;
    cmd_init(&command);
    reg_match_t tokens = match("([`]([^`]+)*[`])|([']([^']+)*['])|([\"]([^\"]+)*[\"])|([^[:space:]'\"]+)", str, 1);
    if(tokens.n_matches < 0) {
        fprintf(stderr, "[parse_command]: No match, aborting...\n");
        exit(EXIT_FAILURE);
    }
    char **parsed = tokens.get_group(&tokens, 0);
    for(int i = 0; i < tokens.n_matches; ++i) {
        if(strcmp(parsed[i], "\0") == 0)
            continue;
        printf("[parse_command]: token: %s, matches: %d, groups: %d\n", parsed[i], tokens.n_matches, tokens.n_groups);
    }
    command.set(&command, parsed, tokens.n_matches);
    for(int i = 0; i < tokens.n_matches; ++i) {
        free(parsed[i]);
    }
    free(parsed);
    tokens.free(&tokens);
    return command;
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
        cmds[i] = parse_command(parsed[i]);
    new_pipe.set(&new_pipe, cmds, tokens.n_matches);
    for(int i = 0; i < tokens.n_matches; ++i)
        free(parsed[i]);
    free(parsed);
    free(cmds);
    tokens.free(&tokens);
    return new_pipe;
}

void run(char *str) {
    pipe_t cmds = parse(str);
    exec(cmds);
    cmds.free(&cmds);
}

int main() {
    heaph_init();
    char *in_str = NULL;
    size_t len = 0;
    ssize_t read;
    while((read = getline(&in_str, &len, stdin)) != EOF) {
//        printf("%s", in_str);
        run(in_str);
    }
    free(in_str);
    printf("%lu", heaph_get_alloc_count());
    return 0;
}
