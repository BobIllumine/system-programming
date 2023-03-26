//
// Created by riseinokoe on 13.03.23.
//
#include <stdio.h>
#include <malloc.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <string.h>
#include "heap_help/heap_help.h"
#include "regex_lib/regex_lib.h"

#define min(a, b) (a < b ? a : b)
#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))
typedef struct regex_match reg_m_t;

int main() {
    heaph_init();
    // (([^[:space:]'"]+)*[[:space:]]*)*
    // ['"](([^'"]+)[[:space]])*['"]
    //(([^[:space:]'"]+)+[[:space:]]+)(['"](([^'"]+)[[:space:]]*)*['"])?
    // (([^[:space:]'"]+)+[[:space:]]+)|

    //(['].*['])|(["].*["])|([^[:space:]'"]+)
    //([`'"]([^`'"]*)[`'"])|([^[:space:]'"]+)
    char pattern[] = "([`]([^`]+)*[`])|([']([^']+)*['])|([\"]([^\"]+)*[\"])|([^[:space:]'\"]+)";
    char str[] = "        ls -i 'h' 'i`hello`' fsfs                        ";
    printf("%s\n", str);
    reg_m_t reg_m = match(pattern, str, 2);
    printf("matches -- %d, size -- %d\n", reg_m.n_matches, reg_m.size);
    char **groups = reg_m.get_group(&reg_m, 0);
    for(int i = 0; i < reg_m.n_matches; ++i) {
        if(strcmp(groups[i], "\0") == 0) {
            printf("wtf\n");
            break;
        }
        printf("%s\n", groups[i]);
        free(groups[i]);
    }
    free(groups);
    reg_m.free(&reg_m);
    printf("memory leaks - %lu", heaph_get_alloc_count());
    return 0;
}
