#include <stdio.h>
#include <malloc.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <string.h>
#include <regex.h>
#include <heap_help.h>
#include <regex_lib.h>
#include <cmd.h>

#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))


typedef struct m_pipe pipe_t;
typedef struct m_cmd cmd_t;
typedef struct regex_match reg_match_t;

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
