#include "sym.h"
#include "decl.h"
#include "inst.h"
#include "lib.h"
#include "parse.h"

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

void collect_syms()
{
    struct symbol **last = &g_syms;
    uint64_t lc = 0; // Location counter

    struct section *currsect = NULL;

    char *line = NULL;
    size_t n = 0;
    int lineno = 0;
    while (getline(&line, &n, g_inf) != -1)
    {
        lineno++;
        if (*line == '\n') continue;
        if (!isspace(*line))
        {
            // Label
            char *lbl = strdup(line);
            *strchr(lbl, ':') = 0;

            *last = calloc(1, sizeof(struct symbol));
            **last = (struct symbol) {
                .name = lbl,
                .val = lc - currsect->offset,
                .sect = currsect
            };
            last = &(*last)->next;
        }
        else if (line[4] == '.')
        {
            if (!strncmp(line + 4, ".str", 4))
            {
                char *direct = line + 10;
                lc += strchr(direct, '"') - direct;
            }
            else if (!strncmp(line + 4, ".section", 8))
            {
                char *strt = line + 13;
                char *name = strndup(strt, strchr(strt, '\n') - strt);
                currsect = addsect(name);
                currsect->offset = lc;
            }
            else if (!strcmp(line + 4, ".byte")) lc++;
            else if (!strcmp(line + 4, ".word")) lc += 2;
            else if (!strcmp(line + 4, ".long")) lc += 4;
            else if (!strcmp(line + 4, ".quad")) lc += 8;
        }
        else if (isalpha(line[4]))
        {
            // Instruction
            struct code code = parse_code(line);
            struct inst *inst = searchi(&code);
            if (!inst)
                error("Line %d: Invalid instruction\n", lineno);
            lc += instsize(inst, &code);
        }
    }

    free(line);
    fseek(g_inf, 0, SEEK_SET);
}

struct symbol *findsym(const char *name)
{
    for (struct symbol *sym = g_syms; sym; sym = sym->next)
        if (!strcmp(sym->name, name)) return sym;
    return NULL;
}

void addsym(struct symbol *sym)
{
    struct symbol *last;
    for (last = g_syms; last->next; last = last->next);
    
    last->next = memdup(sym, sizeof(struct symbol));
}

static int sym_compar(const void *a, const void *b)
{
    int a1 = ((*(struct symbol**)a)->flags & SYM_GLOB);
    int b1 = ((*(struct symbol**)b)->flags & SYM_GLOB);

    return a1 == b1 ? 0 : a1 > b1 ? 1 : -1;
}

// Sort by binding
void sort_symbols()
{
    struct symbol *sym;
    size_t nitems = 0;
    for (sym = g_syms; sym; sym = sym->next) nitems++;

    if (!nitems) return;

    struct symbol **arr = malloc(nitems * sizeof(struct symbol*));
    struct symbol **arrp = arr;
    for (sym = g_syms; sym; sym = sym->next) *arrp++ = sym;

    qsort(arr, nitems, sizeof(struct symbol*), sym_compar);

    for (size_t i = 0; i < nitems - 1; i++) arr[i]->next = arr[i + 1];
    g_syms = arr[0];
    arr[nitems - 1]->next = NULL;

    free(arr);
}

struct section *creatsect(const char *name)
{
    struct section s = { .name = name };
    return memdup(&s, sizeof(struct section));
}

struct section *addsect(const char *name)
{
    struct section **last;
    for (last = &g_sects; (*last); last = &((*last)->next));
    return *last = creatsect(name);
}

struct section *findsect(const char *name)
{
    for (struct section *sect = g_sects; sect; sect = sect->next)
        if (!strcmp(sect->name, name)) return sect;
    return NULL;
}

size_t sectcnt()
{
    size_t i = 0;
    for (struct section *s = g_sects; s; s = s->next) i++;
    return i;
}

size_t sectnum(struct section *sect)
{
    size_t i = 0;
    for (struct section *s = g_sects; s && s != sect; s = s->next) i++;
    return i;
}

struct reloc *sect_add_reloc(struct section *sect, size_t offset, struct symbol *sym, int64_t addend)
{
    struct reloc **last;
    for (last = &sect->rels; *last; last = &((*last)->next));

    struct reloc rel = {
        .sym = sym,
        .addend = addend,
        .offset = offset
    };

    *last = memdup(&rel, sizeof(struct reloc));
    return *last;
}