#include <sym.h>
#include <asm.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// TODO: This function isn't great
void sym_put(struct symtable *tab, const char *name, struct type t, int attr)
{
    size_t stackoff = 0;

    if (tab->type != SYMTAB_GLOB)
    {
        struct symtable *func = tab;
        while (func->type != SYMTAB_FUNC) func = func->parent;
        stackoff = (func->curr_stackoff += asm_sizeof(t));
    }

    tab->syms = realloc(tab->syms, (tab->cnt + 1) * sizeof(struct sym));
    tab->syms[tab->cnt] = (struct sym)
    {
        .attr         = tab->type == SYMTAB_GLOB ? SYM_GLOBAL | attr : SYM_LOCAL | attr,
        .name         = strdup(name),
        .type         = t,
        .stackoff     = stackoff
    };
    tab->cnt++;
}

void sym_putglob(struct symtable *tab, struct sym* sym)
{
    sym->attr |= SYM_GLOBAL;
    tab->syms = realloc(tab->syms, (tab->cnt + 1) * sizeof(struct sym));
    tab->syms[tab->cnt] = *sym;
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