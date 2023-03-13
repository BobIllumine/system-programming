#include <stdio.h>
#include <malloc.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <string.h>
#include "heap_help/heap_help.h"

typedef struct command {
    const char *name;
    char *const *argv;
    const int argc;
} cmd_t;

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
    char *hello_world = NULL;
    size_t len = 0;
    ssize_t read;
    while((read = getline(&hello_world, &len, stdin)) != -1) {
        cmd_t *result = parse_pipe(hello_world);
    }
    free(result);
    free(hello_world);
    printf("%lu", heaph_get_alloc_count());
    return 0;
}
