#include <sym.h>

#include <stdlib.h>
#include <string.h>

static struct symtable s_globtab = { .cnt = 0, .parent = NULL, .syms = NULL };

void sym_putglob(const char *name, struct type t, int attr)
{
    s_globtab.syms = realloc(s_globtab.syms, (s_globtab.cnt + 1) * sizeof(struct sym));
    s_globtab.syms[s_globtab.cnt] = (struct sym)
    {
        .attr     = SYM_GLOBAL | attr,
        .name     = strdup(name),
        .type     = t,
        .stackoff = 0
    };
    s_globtab.cnt++;
}

struct sym *sym_lookup(const char *name)
{
    for (unsigned int i = 0; i < s_globtab.cnt; i++)
        if (!strcmp(name, s_globtab.syms[i].name)) return &s_globtab.syms[i];

    return NULL;
}