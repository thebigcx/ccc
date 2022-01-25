#pragma once

#include <stdint.h>
#include <stddef.h>

#define SYM_GLOB  (1 << 0)
#define SYM_SECT  (1 << 1)
#define SYM_UNDEF (1 << 2) // Undefined

struct section;

struct symbol
{
    char *name;
    uint64_t val;
    int namei; // Name index into string section
    int flags; // Flags
    struct section *sect;
    struct symbol *next; // Next symbol in linked-list
};

void collect_syms();
struct symbol *findsym(const char *name);
void addsym(struct symbol *sym);
void sort_symbols();

struct reloc
{
    struct symbol *sym;
    int64_t addend;
    unsigned int offset;

    struct reloc *next;
};

struct section
{
    const char *name;   // Name
    struct reloc *rels; // Relocations
    unsigned int offset, size;
    int namei;

    struct section *next;
};

struct section *addsect(const char *name);
struct section *findsect(const char *name);
struct section *creatsect(const char *name);
size_t sectcnt();
size_t sectnum(struct section *sect);

struct reloc *sect_add_reloc(struct section *sect, size_t offset, struct symbol *sym, int64_t addend);
