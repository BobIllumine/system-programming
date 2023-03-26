//
// Created by riseinokoe on 23.03.23.
//

#include "regex_lib.h"
#include <malloc.h>
#include <string.h>

#define min(a, b) (a < b ? a : b)

char** reg_get_group(struct regex_match *reg_m, int group) {
    char **reg_group = (char**)malloc(reg_m->n_matches * sizeof(char*));
    for(int i = 0; i < reg_m->n_matches; ++i) {
        if(reg_m->groups[i][group].rm_so == -1 && reg_m->groups[i][group].rm_eo == -1) {
            reg_group[i] = strdup("\0");
            continue;
        }
        regoff_t off = reg_m->groups[i][group].rm_so, len = reg_m->groups[i][group].rm_eo - off;
        reg_group[i] = strndup(reg_m->str + off, len);
    }
    return reg_group;
}
void reg_match_free(struct regex_match *reg_m) {
    for(int i = 0; i < reg_m->n_matches; ++i)
        free(reg_m->groups[i]);
    free(reg_m->groups);
    free(reg_m->str);
}
void reg_add(struct regex_match *reg_m, regmatch_t *matched) {
    if(reg_m->n_matches + 1 == reg_m->size) {
        reg_m->groups = (regmatch_t **) realloc(reg_m->groups, sizeof(regmatch_t *) * reg_m->size * 2);
        reg_m->size *= 2;
    }
    reg_m->groups[reg_m->n_matches] = (regmatch_t *) malloc(sizeof(regmatch_t) * reg_m->n_groups);
    for(int i = 0; i < reg_m->n_groups; ++i) {
        regmatch_t tmp;
        tmp.rm_so = -1, tmp.rm_eo = -1;
        reg_m->groups[reg_m->n_matches][i] = matched[i];
//        printf("[reg_add]: i = %d, rm_so = %lu, rm_eo = %lu\n", i, matched[i].rm_so, matched[i].rm_eo);
    }
    reg_m->n_matches++;
}

void reg_init(struct regex_match *reg_m, int n_groups, char *init_str) {
    reg_m->free = &reg_match_free;
    reg_m->get_group = &reg_get_group;
    reg_m->add = &reg_add;
    reg_m->n_groups = n_groups;
    reg_m->size = 1;
    reg_m->n_matches = 0;
    reg_m->str = strdup(init_str);
    reg_m->groups = (regmatch_t **) malloc(sizeof(regmatch_t*));
}

struct regex_match match(const char *pattern, char *str, int n_groups) {
    regex_t regex;
    regmatch_t pmatch[n_groups + 1];
    regoff_t off, len;

    struct regex_match result;
    reg_init(&result, n_groups + 1, str);
//    printf("[match] String: %s\n", str);

    if(regcomp(&regex, pattern, REG_EXTENDED | REG_NEWLINE)) {
        fprintf(stderr, "[match] Could not compile regex\n");
        result.n_matches = -1;
        return result;
    }
    char *s = strdup(str), *p_s = s;
    regoff_t universal_off = 0;
    for(int i = 0; ; ++i) {
        if (regexec(&regex, p_s, n_groups + 1, pmatch, 0) == REG_NOMATCH)
            break;
        int g;
        universal_off = (p_s - s);
        for (g = 0; g < n_groups + 1; ++g) {
            if(pmatch[g].rm_so == (size_t)-1) {
//                printf("[match]: last group: %d\n", g);
                break;
            }
            char *copy = strdup(p_s);
            copy[pmatch[g].rm_eo] = 0;
//            printf("[match]: run %d, group %d: [%llu-%llu, %llu] -- \"%s\"\n", i, g, pmatch[g].rm_so, pmatch[g].rm_eo, universal_off, copy + pmatch[g].rm_so);
            free(copy);
        }
        p_s += pmatch[0].rm_eo;
        for(int j = 0; j < n_groups + 1; ++j) {
            if(pmatch[j].rm_so == -1)
                pmatch[j].rm_eo = -1;
            else
                pmatch[j].rm_so += universal_off, pmatch[j].rm_eo += universal_off;
        }
        result.add(&result, pmatch);
    }
    regfree(&regex);
    return result;
}