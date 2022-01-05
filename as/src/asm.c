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
    [REG_AH]  = 0b0100,
    [REG_CH]  = 0b0101,
    [REG_DH]  = 0b0110,
    [REG_BH]  = 0b0111,

    [REG_AL]  = 0b0000,
    [REG_CL]  = 0b0001,
    [REG_DL]  = 0b0010,
    [REG_BL]  = 0b0011,
    [REG_SPL] = 0b0100,
    [REG_BPL] = 0b0101,
    [REG_SIL] = 0b0110,
    [REG_DIL] = 0b0111,

    [REG_AX]  = 0b0000,
    [REG_CX]  = 0b0001,
    [REG_DX]  = 0b0010,
    [REG_BX]  = 0b0011,
    [REG_SP]  = 0b0100,
    [REG_BP]  = 0b0101,
    [REG_SI]  = 0b0110,
    [REG_DI]  = 0b0111,

    [REG_EAX] = 0b0000,
    [REG_ECX] = 0b0001,
    [REG_EDX] = 0b0010,
    [REG_EBX] = 0b0011,
    [REG_ESP] = 0b0100,
    [REG_EBP] = 0b0101,
    [REG_ESI] = 0b0110,
    [REG_EDI] = 0b0111,
   
    [REG_RAX] = 0b0000,
    [REG_RCX] = 0b0001,
    [REG_RDX] = 0b0010,
    [REG_RBX] = 0b0011,
    [REG_RSP] = 0b0100,
    [REG_RBP] = 0b0101,
    [REG_RSI] = 0b0110,
    [REG_RDI] = 0b0111,
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

// REX required registers: SPL, BPL, SIL, DIL
#define REXREQR(r) (r == REG_SPL || r == REG_BPL || r == REG_SIL || r == REG_DIL)

// Is REX prefix required
int isrexreq(struct code *code)
{
    if (code->size == 64) return 1;

    if (code->op1 && code->op1->type == CODE_REG && REXREQR(code->op1->reg)) return 1;
    if (code->op2 && code->op2->type == CODE_REG && REXREQR(code->op2->reg)) return 1;

    return 0;
}

int regsize(int reg)
{
    if (reg >= REG_AH  && reg <= REG_DIL) return 8;
    if (reg >= REG_AX  && reg <= REG_DI)  return 16;
    if (reg >= REG_EAX && reg <= REG_EDI) return 32;
    if (reg >= REG_RAX && reg <= REG_RDI) return 64;

    return 0;
}

// High bits of opcode
uint8_t opcodehi[] =
{
    [INST_ADD] = 0
};

void do_inst(struct code *code)
{
    if (code->size == 16)
        emitb(0x66);
    
    if (isrexreq(code))
        emitb(rexpre(code->size == 64, regcodes[code->op1->reg] & 0b1000, 0, regcodes[code->op2->reg] & 0b1000));

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
