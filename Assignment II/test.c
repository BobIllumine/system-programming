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

int main() {
    heaph_init();
    char **strs = (char**)malloc(2 * sizeof(char*));
    char strings[3][5] = {"helo", "wold", "wowu"};
    int len = 2, cur_id = 0;
//    strs = (char**)realloc(strs, len * 2 * sizeof(char*));
//    len *= 2;
    while(cur_id < 3) {
        if(cur_id == len - 1) {
            strs = (char**)realloc(strs, len * 2 * sizeof(char*));
            len *= 2;
        }
        *strs = strdup(strings[cur_id]);
        cur_id++;
        ++strs;
    }
    for(int i = 0; i < 3; ++i) {
        printf("%s\n", strs[i]);
        free(strs[i]);
    }
    free(strs);
    printf("memory leaks u stoopid bicht - %lu", heaph_get_alloc_count());
    return 0;
}
