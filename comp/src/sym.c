#include <sym.h>
#include <asm.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

void sym_put(struct symtable *tab, const char *name, struct type t, int attr)
{
    size_t stackoff = 0;

    if (!tab->global)
        stackoff = (tab->curr_stackoff += asm_sizeof(t));

    tab->syms = realloc(tab->syms, (tab->cnt + 1) * sizeof(struct sym));
    tab->syms[tab->cnt] = (struct sym)
    {
        .attr     = tab->global ? SYM_GLOBAL | attr : SYM_LOCAL | attr,
        .name     = strdup(name),
        .type     = t,
        .stackoff = stackoff
    };
    tab->cnt++;
}

struct sym *sym_lookup(struct symtable *curr, const char *name)
{
    if (!curr) return NULL;

    for (unsigned int i = 0; i < curr->cnt; i++)
        if (!strcmp(name, curr->syms[i].name)) return &curr->syms[i];

    // Recurse through the parent scopes
    return sym_lookup(curr->parent, name);
}