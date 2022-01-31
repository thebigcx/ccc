#include "asm.h"
#include "elf.h"
#include "sym.h"
#include "inst.h"
#include "decl.h"
#include "lib.h"
#include "parse.h"

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

void emit8(uint8_t v)
{
    fwrite(&v, 1, 1, g_outf);
}

void emit16(uint16_t v)
{
    emit8(v & 0xff);
    emit8(v >> 8);
}

void emit32(uint32_t v)
{
    emit16(v & 0xffff);
    emit16(v >> 16);
}

void emit64(uint64_t v)
{
    emit32(v & 0xffffffff);
    emit32(v >> 32);
}

void emit(int size, uint64_t v)
{
    switch (size)
    {
        case OP_SIZE8:  emit8(v);  break;
        case OP_SIZE16: emit16(v); break;
        case OP_SIZE32: emit32(v); break;
        case OP_SIZE64: emit64(v); break;
    }
}

struct inst *searchi(struct code *code, struct inst *tbl, size_t len)
{
    for (size_t i = 0; i < len; i++)
    {
        int op1u = code->op1.type & OP_TYPEM;
        int op2u = code->op2.type & OP_TYPEM;
        if (!strcmp(tbl[i].mnem, code->mnem)
                && (!op1u || (code->op1.type & OP_TYPEM) & (tbl[i].op1 & OP_TYPEM))
                && (!op1u || (code->op1.type & OP_SIZEM) & (tbl[i].op1 & OP_SIZEM))
                && (!op2u || (code->op2.type & OP_TYPEM) & (tbl[i].op2 & OP_TYPEM))
                && (!op2u || (code->op2.type & OP_SIZEM) & (tbl[i].op2 & OP_SIZEM)))
            return &tbl[i];
    }

    return NULL;
}

void assemble_file()
{
    char *line = NULL;
    size_t n = 0;

    uint64_t lc = 0;

    g_currsect = NULL;

    elf_begin_file();

    while (getline(&line, &n, g_inf) != -1)
    {
        if (*line == '\n') continue;
        if (isalpha(*line) || *line == '_') continue;

        char *strt = *line == '\t' ? line + 1 : line + 4;
        if (*strt == '.')
        {
            char *direct = strndup(strt, strchr(strt, ' ') - strt);
            if (!strcmp(direct, ".section"))
            {
                strt += strlen(direct) + 1;
                char *name = strndup(strt, strchr(strt, '\n') - strt);
           
                if (g_currsect) g_currsect->size = ftell(g_outf) - g_currsect->offset;

                g_currsect = findsect(name);
                g_currsect->offset += 64; // sizeof(Elf64_Ehdr) TODO: don't do this

                struct symbol sym = {
                    .name = name,
                    .flags = SYM_SECT,
                    .sect = g_currsect
                };
                addsym(&sym);
            }
            else if (!strcmp(direct, ".global"))
            {
                strt += strlen(direct) + 1;
                char *name = strndup(strt, strchr(strt, '\n') - strt);

                findsym(name)->flags |= SYM_GLOB;

                free(name);
            }
            else if (!strncmp(strt, ".type", 5))
            {
                char *sym = strt + 6;
                *strchr(sym, ',') = 0;

                char *type = sym + strlen(sym) + 2;
                *strchr(type, '\n') = 0;

                findsym(sym)->type = symtypestr(type);
            }
            else if (!strncmp(strt, ".size", 5))
            {
                char *sym = strt + 6;
                *strchr(sym, ',') = 0;

                char *size = sym + strlen(sym) + 2;
                findsym(sym)->size = xstrtonum(size, NULL);
            }
            else if (!strcmp(direct, ".str"))
            {
                strt += strlen(direct) + 2;
                char *str = stresc(strt);

                fputs(str, g_outf);
                fputc(0, g_outf);

                free(str);
            }
            else if (!strcmp(direct, ".byte"))
            {
                strt += strlen(direct) + 1;
                emit8(xstrtonum(strt, NULL));
            }
            else if (!strcmp(direct, ".word"))
            {
                strt += strlen(direct) + 1;
                emit16(xstrtonum(strt, NULL));
            }
            else if (!strcmp(direct, ".long"))
            {
                strt += strlen(direct) + 1;
                emit32(xstrtonum(strt, NULL));
            }
            else if (!strcmp(direct, ".quad"))
            {
                strt += strlen(direct) + 1;
                emit64(xstrtonum(strt, NULL));
            }
            else if (!strcmp(direct, ".skip"))
            {
                strt += strlen(direct) + 1;
                size_t s = xstrtonum(strt, NULL);

                for (size_t i = 0; i < s; i++)
                    emit8(0);
            }
            else if (!strcmp(direct, ".code16"))
                g_currsize = 16;
            else if (!strcmp(direct, ".code64"))
                g_currsize = 64;

            free(direct);
            continue;
        }

        //if (g_currsize == 64)
        //{
            struct code code = parse_code(strt);
            struct inst *ent = searchi64(&code);
            if (!ent)
                error("Invalid instruction\n");

            lc += instsize64(ent, &code);

            assemble64(&code, ent, lc);
        /*}
        else if (g_currsize == 16)
        {
            struct code code = parse_code(strt);
            struct inst *ent = searchi16(&code);
            if (!ent)
                error("Invalid instruction\n");

            lc += instsize16(ent, &code);

            assemble16(&code, ent, lc);
        }*/
    }

    if (g_currsect) g_currsect->size = ftell(g_outf) - g_currsect->offset;
    elf_end_file();
}
