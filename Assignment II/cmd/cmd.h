//
// Created by riseinokoe on 26.03.23.
//

#ifndef ASSIGNMENT_II_CMD_H
#define ASSIGNMENT_II_CMD_H

struct m_cmd {
    char *name;
    char **argv;
    int argc;
    void (*set)(struct m_cmd*, char**, int);
    void (*free)(struct m_cmd*);
};

struct m_pipe {
    struct m_cmd *commands;
    int size;
    void (*set)(struct m_pipe*, struct m_cmd*, int);
    void (*free)(struct m_pipe*);
};

void cmd_init(struct m_cmd*);

void pipe_init(struct m_pipe*);

void exec_cmd(struct m_cmd, int);

void exec(struct m_pipe);

struct m_cmd parse_cmd(char*);

struct m_pipe parse(char*);

#endif //ASSIGNMENT_II_CMD_H
