#pragma once

#include <parser.h>

#include <stdint.h>
#include <stddef.h>

#define SYM_LOCAL  (0b0001)
#define SYM_GLOBAL (0b0010)
#define SYM_STATIC (0b0100)
#define SYM_EXTERN (0b1000)

struct ast;

struct sym
{
    int attr;
    char *name;
    size_t stackoff;
    struct type type;
};

struct symtable
{
    struct sym      *syms;
    unsigned int    cnt;
    struct symtable *parent;
};

//void sym_putlocal(struct ast *scope, const char *name);
void sym_putglob(const char *name, struct type t, int attr);

struct sym *sym_lookup(const char *name);