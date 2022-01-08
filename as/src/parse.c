#include <parse.h>
#include <lexer.h>

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

static struct token *s_t = NULL;

uint8_t regcodes[] =
{   
    [LREG_AH]  = 0b0100,
    [LREG_CH]  = 0b0101,
    [LREG_DH]  = 0b0110,
    [LREG_BH]  = 0b0111,

    [LREG_AL]  = 0b0000,
    [LREG_CL]  = 0b0001,
    [LREG_DL]  = 0b0010,
    [LREG_BL]  = 0b0011,
    [LREG_SPL] = 0b0100,
    [LREG_BPL] = 0b0101,
    [LREG_SIL] = 0b0110,
    [LREG_DIL] = 0b0111,

    [LREG_AX]  = 0b0000,
    [LREG_CX]  = 0b0001,
    [LREG_DX]  = 0b0010,
    [LREG_BX]  = 0b0011,
    [LREG_SP]  = 0b0100,
    [LREG_BP]  = 0b0101,
    [LREG_SI]  = 0b0110,
    [LREG_DI]  = 0b0111,

    [LREG_EAX] = 0b0000,
    [LREG_ECX] = 0b0001,
    [LREG_EDX] = 0b0010,
    [LREG_EBX] = 0b0011,
    [LREG_ESP] = 0b0100,
    [LREG_EBP] = 0b0101,
    [LREG_ESI] = 0b0110,
    [LREG_EDI] = 0b0111,
   
    [LREG_RAX] = 0b0000,
    [LREG_RCX] = 0b0001,
    [LREG_RDX] = 0b0010,
    [LREG_RBX] = 0b0011,
    [LREG_RSP] = 0b0100,
    [LREG_RBP] = 0b0101,
    [LREG_RSI] = 0b0110,
    [LREG_RDI] = 0b0111,
};

int uint_size(unsigned long i)
{
    return i < UINT8_MAX ? 8 : i < UINT16_MAX ? 16 : i < UINT32_MAX ? 32 : 64;
}

void error(const char *format, ...)
{
    va_list list;
    va_start(list, format);
    vprintf(format, list);
    va_end(list);
    abort();
    exit(-1);
}

void expect(int t)
{
    if ((s_t++)->type != t)
        error("Expected token\n");
}

int size_prefix()
{
    switch (s_t->type)
    {
        case T_U8:  return 8;
        case T_U16: return 16;
        case T_U32: return 32;
        case T_U64: return 64;
    }
    return 0;
}

int regsize(int reg)
{
    if (reg >= LREG_AH  && reg <= LREG_DIL) return 8;
    if (reg >= LREG_AX  && reg <= LREG_DI)  return 16;
    if (reg >= LREG_EAX && reg <= LREG_EDI) return 32;
    if (reg >= LREG_RAX && reg <= LREG_RDI) return 64;

    return 0;
}

void parse_addr(struct code *code)
{
    code->type = CODE_ADDR;
    code->addr.base  = -1;
    code->addr.index = -1;

    expect(T_LBRACK);
    if (s_t->type == T_IMM)
    {
        code->addr.isdisp = 1;
        code->addr.disp = s_t->ival;
        code->size = 32;
        s_t++;
        expect(T_RBRACK);
        return;
    }
    else if (s_t->type == T_LPAREN)
        goto index;

    code->addr.base = regcodes[s_t->ival];
    code->size = regsize(s_t->ival);
    expect(T_REG);
    
    if (s_t->type == T_RBRACK)
    {
        s_t++;
        return;
    }

    expect(T_PLUS);

    if (s_t->type == T_IMM)
    {
        code->addr.isdisp = 1;
        code->addr.disp = s_t->ival;
        s_t++;
        expect(T_RBRACK);
        return;
    }

index:
    expect(T_LPAREN);

    code->addr.index = regcodes[s_t->ival];
    code->size = regsize(s_t->ival);
    expect(T_REG);
    expect(T_STAR);
    
    code->addr.scale = s_t->ival;
    expect(T_IMM);
    expect(T_RPAREN);

    if (s_t->type == T_RBRACK)
    {
        s_t++;
        return;
    }

    expect(T_PLUS);
    code->addr.isdisp = 1;
    code->addr.disp = s_t->ival;
    expect(T_IMM);
    expect(T_RBRACK);
}

struct code *parse()
{
    struct code *code = calloc(1, sizeof(struct code));

    switch (s_t->type)
    {
        case T_LBL:
            code->type = CODE_LBL;
            code->sval = strdup(s_t->sval);
            s_t++;
            break;

        case T_INST:
            code->inst = s_t->ival;
            code->type = CODE_INST;
            
            s_t++;
            if ((code->size = size_prefix())) s_t++;

            if (s_t->type != T_INST && s_t->type != T_EOF)
                code->op1 = parse();
            
            if (s_t->type == T_COMMA)
            {
                expect(T_COMMA);
                code->op2 = parse();
            }
            
            if (!code->size && code->op1)
            {
                if (!code->op2 && code->op1->type == CODE_IMM) code->size = uint_size(code->op1->imm);
                else if (code->op1 && code->op1->type == CODE_REG) code->size = code->op1->size;
                else if (code->op2 && code->op2->type == CODE_REG) code->size = code->op2->size;
                else error("Could not deduce instruction size.\n");
            }
            break;

        case T_REG:
            code->type = CODE_REG;
            code->size = regsize(s_t->ival);
            code->reg  = regcodes[s_t->ival];
            s_t++;
            break;

        case T_IMM:
            code->type = CODE_IMM;
            code->imm  = s_t->ival;
            code->size = uint_size(code->imm);
            s_t++;
            break;

        case T_LBRACK: parse_addr(code); break;
    }

    return code;
}

int parse_file(struct code ***code, size_t *codecnt, struct token *toks)
{
    s_t = toks;

    while (s_t->type != T_EOF)
    {
        *code = realloc(*code, (*codecnt + 1) * sizeof(struct code*));
        (*code)[(*codecnt)++] = parse();
    }

    return 0;
}
