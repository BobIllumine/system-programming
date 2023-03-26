//
// Created by riseinokoe on 23.03.23.
//
#include <regex.h>

#ifndef ASSIGNMENT_II_REGEX_LIB_H
#define ASSIGNMENT_II_REGEX_LIB_H

struct regex_match {
    char *str;
    regmatch_t **groups;
    int n_matches;
    int n_groups;
    int size;
    void (*free)(struct regex_match*);
    void (*add)(struct regex_match*, regmatch_t*);
    char** (*get_group)(struct regex_match*, int);
};

void reg_init(struct regex_match*, int, char*);

struct regex_match match(const char*, char*, int);

#endif //ASSIGNMENT_II_REGEX_LIB_H
