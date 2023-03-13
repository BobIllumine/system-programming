#include <stdio.h>
#include <malloc.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <string.h>
#include "heap_help/heap_help.h"

typedef struct command {
    const char *name;
    char **argv;
    int argc;
} cmd_t;

cmd_t cmd(char **tokens, int len) {
    cmd_t command;
    command.argc = len;
    command.name = strdup(tokens[0]);
    command.argv = (char**)malloc(len * sizeof(char*));
    for(int i = 0; i < len; ++i)
        command.argv[i] = strdup(tokens[i]);
    return command;
}

void exec_cmd(cmd_t command) {
    pid_t pid = fork();
    if(pid == -1) {
        printf("Shit's bad");
    }
    else if(pid == 0) {
        if(execvp(command.name, command.argv) < 0) {
            printf("Something went bad");
        }
        exit(0);
    }
    else {
        wait(NULL);
        return;
    }
}

void exec_piped_cmd(cmd_t cmd1) {}

cmd_t parse_command(char *str) {
    cmd_t command;
    char delim[] = " ";
    char *full_str = str,
         **tokens = (char**) malloc(2 * sizeof(char*)),
         *cur_token = NULL;
    int len = 2, cur_id = 0;
    while((cur_token = strsep(&full_str, delim)) != NULL) {
        printf("%s, ", cur_token);
        if(len - 1 == cur_id) {
            tokens = (char**) realloc(tokens,  len * 2 * sizeof(char*));
            len *= 2;
        }
        tokens[cur_id] = strdup(cur_token);
        ++cur_id;
    }
    if(strcmp(tokens[cur_id - 1], "\n") == 0) --len;
    command = cmd(tokens, cur_id);
    for(int i = 0; i <= len; ++i)
        free(tokens[i]);
    free(tokens);
    free(cur_token);
    return command;
}

cmd_t* parse_pipe(char *str) {
    cmd_t *commands = NULL;
    char delim[] = "|";
    char *full_str = str;
    char *token = NULL;
    while((token = strsep(&full_str, delim)) != NULL) {
        printf("%s, ", token);
    }
    free(token);
    return commands;
}

int main() {
    heaph_init();
    char *in_str = NULL;
    size_t len = 0;
    ssize_t read;
    while((read = getline(&in_str, &len, stdin)) != -1) {
//        printf("%s", in_str);
        cmd_t result = parse_command(in_str);
        printf("argc -- %d, name -- %s", result.argc, result.name);
        exec_cmd(result);
    }
    free(in_str);
    printf("%lu", heaph_get_alloc_count());
    return 0;
}
