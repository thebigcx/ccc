#include <asm.h>
#include <defs.h>
#include <lexer.h>

#include <stdarg.h>
#include <stdint.h>

static FILE *s_out = NULL;
static struct token *s_t;

uint8_t regcodes[] =
{
    [REG_RAX] = 0b1000,
    [REG_RCX] = 0b1001,
    [REG_RDX] = 0b1010,
    [REG_RBX] = 0b1011
};

void error(const char *format, ...)
{
    va_list list;
    va_start(list, format);
    vprintf(format, list);
    va_end(list);
}

void emitb(uint8_t b)
{
    fwrite(&b, 1, 1, s_out);
}

void emitw(uint16_t b)
{
    fwrite(&b, 2, 1, s_out);
}

uint8_t rexpre(int is64)
{
    return 0b01000000 | (!!is64 << 3);
}

int regsize(int reg)
{
    if (reg >= REG_AH  && reg <= REG_DIL) return 8;
    if (reg >= REG_AX  && reg <= REG_DI)  return 16;
    if (reg >= REG_EAX && reg <= REG_EDI) return 32;
    if (reg >= REG_RAX && reg <= REG_RDI) return 64;

    return 0;
}

void do_inst(int type)
{
    switch (type)
    {
        case INST_ADD:
        {
            s_t++;
            struct token *op1 = s_t++;
            ++s_t; // TODO: expect T_COMMA
            struct token *op2 = s_t++;

            if (op1->type == T_REG && op2->type == T_REG)
            {
                if (regsize(op1->ival) != regsize(op2->ival)) error("Mismatched register sizes\n");
                if (regsize(op1->ival) == 64) emitb(rexpre(1));
                emitb(0x01);
                emitb(0b11000000 | (regcodes[op1->ival] & 0b111) << 3 | (regcodes[op2->ival] & 0b111));
                break;
            }
        }
    }
}

int assemble(FILE *out, struct token *toks)
{
    s_t   = toks;
    s_out = out;

    while (s_t->type != T_EOF)
    {
        switch (s_t->type)
        {
            case T_LBL: break;
            case T_INST: do_inst(s_t->ival); break;
        }
    }

    return 0;
}
