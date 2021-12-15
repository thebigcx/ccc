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

/*

#define SYM_STATIC (0b0001)
#define SYM_EXTERN (0b0010)
#define SYM_LOCAL  (0b0100)
#define SYM_GLOBAL (0b1000)

#define SYM_VAR  1
#define SYM_FUNC 2

struct sym
{
    int type, attr;
    char *name;

    struct
    {
        struct type type;
        size_t stackoff;
    } var;

    struct
    {
        struct type ret;
        struct type params[6];
    } func;
}
*/

struct sym
{
    int attr;
    char *name;

    union
    {
        struct
        {
            struct type type;
            size_t stackoff;
        } var;

        struct
        {
            struct type ret;
            struct type params[6];
            unsigned int paramcnt;
        } func;
    };
};

#define SYMTAB_GLOB  1 // Global symbol table
#define SYMTAB_FUNC  2 // Function symbol table
#define SYMTAB_BLOCK 3 // Block inside function

struct symtable
{
    struct sym      *syms;
    unsigned int    cnt;
    struct symtable *parent;
    size_t          curr_stackoff;
    int             type;
};

extern struct symtable g_globsymtab;

void sym_put(struct symtable *tab, const char *name, struct type t, int attr);
void sym_putglob(struct symtable *tab, struct sym* sym);

struct sym *sym_lookup(struct symtable *curr, const char *name);