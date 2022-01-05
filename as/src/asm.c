#include <asm.h>
#include <defs.h>
#include <lexer.h>

#include <stdarg.h>
#include <stdint.h>

#define ASSERT_REGMATCH(r1, r2) if (regsize(r1) != regsize(r2)) error("Mismatched register sizes.\n")

static FILE *s_out = NULL;
static struct token *s_t;

uint8_t regcodes[] =
{
    [REG_RAX] = 0b0000,
    [REG_RCX] = 0b0001,
    [REG_RDX] = 0b0010,
    [REG_RBX] = 0b0011,

    [REG_AX] = 0b0000,
    [REG_CX] = 0b0001,
    [REG_DX] = 0b0010,
    [REG_BX] = 0b0011,

    [REG_EAX] = 0b0000,
    [REG_ECX] = 0b0001,
    [REG_EDX] = 0b0010,
    [REG_EBX] = 0b0011
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

uint8_t rexpre(int w, int r, int x, int b)
{
    return 0b01000000 | (!!w << 3) | (!!r << 2) | (!!x << 1) | !!b;
}

int regsize(int reg)
{
    if (reg >= REG_AH  && reg <= REG_DIL) return 8;
    if (reg >= REG_AX  && reg <= REG_DI)  return 16;
    if (reg >= REG_EAX && reg <= REG_EDI) return 32;
    if (reg >= REG_RAX && reg <= REG_RDI) return 64;

    return 0;
}

#define R64(r) (regsize(r) == 64)

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
                unsigned int size = regsize(op1->ival);
                uint8_t r1 = regcodes[op1->ival], r2 = regcodes[op2->ival];

                ASSERT_REGMATCH(op1->ival, op2->ival);

                if (size == 16)
                    emitb(0x66);

                if (R64(op1->ival) || r1 & 0b1000 || r2 & 0b1000)                
                    emitb(rexpre(R64(op1->ival), r1 & 0b1000, 0, r2 & 0b1000));
                
                emitb(0x01);
                emitb(0b11000000 | (r1 & 0b111) << 3 | (r2 & 0b111));
                
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
