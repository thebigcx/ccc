#pragma once

#include <type.h>

#include <stdint.h>
#include <stddef.h>

// Variables
#define SYM_LOCAL   (0b000001)
#define SYM_GLOBAL  (0b000010)
#define SYM_STATIC  (0b000100)

// Both
#define SYM_EXTERN  (0b001000)

// Functions
#define SYM_PUBLIC  (0b010000)

struct ast;

struct sym
{
    int attr;
    char *name;
    struct type type;
    size_t stackoff; // If local
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