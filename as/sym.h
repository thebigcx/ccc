#pragma once

#include <stdint.h>
#include <stddef.h>
#include <elf.h>

#define SYM_GLOB  (1 << 0)
#define SYM_SECT  (1 << 1)
#define SYM_UNDEF (1 << 2) // Undefined

#define SYMT_NOTYPE 0
#define SYMT_FUNC   1
#define SYMT_OBJECT 2
#define SYMT_FILE   3

struct section;

struct symbol
{
    char *name;
    uint64_t val;
    int type;
    int namei; // Name index into string section
    int flags; // Flags
    size_t size;
    struct section *sect;
    struct symbol *next; // Next symbol in linked-list
};

int symtypestr(const char *str);

void collect_syms();
struct symbol *findsym(const char *name);
struct symbol *addsym(struct symbol *sym);
void sort_symbols();

#define REL_PC32  R_X86_64_PC32
#define REL_64    R_X86_64_64
#define REL_32S   R_X86_64_32S
#define REL_16    R_X86_64_16
#define REL_PLT   R_X86_64_PLT32

struct reloc
{
    struct symbol *sym;
    int64_t addend;
    unsigned int offset;
    int flags;
    
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

struct reloc *sect_add_reloc(struct section *sect, size_t offset, struct symbol *sym, int64_t addend, int flags);
