#include <asm.h>
#include <defs.h>
#include <lexer.h>

#include <stdarg.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#define ASSERT_REGMATCH(r1, r2) if (regsize(r1) != regsize(r2)) error("Mismatched register sizes.\n")

static FILE *s_out = NULL;
static struct token *s_t = NULL;

static struct code **s_code = NULL;
static unsigned int s_codecnt = 0;

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

void expect(int t)
{
    if ((s_t++)->type != t)
        error("Expected token\n");
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

/*void do_inst(int type)
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

        case INST_MOV:
        {
            s_t++;
            struct token *op1 = s_t++;
            ++s_t;
            struct token *op2 = s_t++;

            if (op1->type == T_REG && op2->type == T_REG)
            {
                
            }
        }
    }
}*/

// High bits of opcode
uint8_t opcodehi[] =
{
    [INST_ADD] = 0
};

void do_inst(struct code *code)
{
    if (code->size == 16)
        emitb(0x66);
    
    if (code->size == 64)
        emitb(rexpre(1, regcodes[code->op1->reg] & 0b1000, 0, regcodes[code->op2->reg]));

    uint8_t opcode = opcodehi[code->inst] << 2;

    if (code->op2->type == CODE_ADDR)
        opcode |= (1 << 1);
    if (code->size != 8)
        opcode |= 1;

    emitb(opcode);

    // TODO: makes assumptions that both reg operands
    emitb(0b11000000 | (regcodes[code->op1->reg] & 0b111) << 3 | (regcodes[code->op2->reg] & 0b111));
}

struct code *parse()
{
    struct code *code = malloc(sizeof(struct code));

    switch (s_t->type)
    {
        case T_LBL:
            code->type = CODE_LBL;
            code->sval = strdup(s_t->sval);
            s_t++;
            break;

        case T_INST:
            s_t++;
            code->type = CODE_INST;
            code->op1 = parse();
            expect(T_COMMA);
            code->op2 = parse();
            code->size = code->op2->size;
            break;

        case T_REG:
            code->type = CODE_REG;
            code->reg  = s_t->ival;
            code->size = regsize(code->reg);
            s_t++;
            break;
    }

    return code;
}

int assemble(FILE *out, struct token *toks)
{
    s_t   = toks;
    s_out = out;

    while (s_t->type != T_EOF)
    {
        s_code = realloc(s_code, (s_codecnt + 1) * sizeof(struct code*));
        s_code[s_codecnt++] = parse();
    }

    for (unsigned int i = 0; i < s_codecnt; i++)
    {
        switch (s_code[i]->type)
        {
            case CODE_INST: do_inst(s_code[i]); break;
        }
    }

    return 0;
}
