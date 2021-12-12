#pragma once

#include <type.h>

#include <stdint.h>
#include <stddef.h>

#define SYM_LOCAL  (0b000001)
#define SYM_GLOBAL (0b000010)
#define SYM_STATIC (0b000100)
#define SYM_EXTERN (0b001000)
#define SYM_VAR    (0b010000)
#define SYM_FUNC   (0b100000)

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
    size_t          curr_stackoff;
    int             global; // Is the global symbol table
};

extern struct symtable g_globsymtab;

void sym_put(struct symtable *tab, const char *name, struct type t, int attr);

struct sym *sym_lookup(struct symtable *curr, const char *name);