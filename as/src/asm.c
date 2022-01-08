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

void emitb(uint8_t b)
{
    fwrite(&b, 1, 1, s_out);
}

// Emit words, longs, and quadwords with proper endianess/ordering
// e.g. 0xdeadbeef as an immediate encodes to FE BE   AD DE
void emitw(uint16_t w)
{
    emitb(w & 0xff);
    emitb(w >> 8);
}

void emitl(uint32_t l)
{
    emitw(l & 0xffff);
    emitw(l >> 16);
}

void emitq(uint64_t q)
{
    emitl(q & 0xffffffff);
    emitl(q >> 32);
}

void emit(uint64_t v, int size)
{
    switch (size)
    {
        case 8:  emitb(v & 0xff);       return;
        case 16: emitw(v & 0xffff);     return;
        case 32: emitl(v & 0xffffffff); return;
        case 64: emitq(v);              return;
    }
}

// REX required uniform byte registers: SPL, BPL, SIL, DIL
#define UNIBREG(c) (c->type == CODE_REG && c->size == 8 && (c->reg == REG_SP || c->reg == REG_BP || c->reg == REG_SI || c->reg == REG_DI))

int regsize(int reg)
{
    if (reg >= LREG_AH  && reg <= LREG_DIL) return 8;
    if (reg >= LREG_AX  && reg <= LREG_DI)  return 16;
    if (reg >= LREG_EAX && reg <= LREG_EDI) return 32;
    if (reg >= LREG_RAX && reg <= LREG_RDI) return 64;

    return 0;
}

int immsize(unsigned long imm)
{
    return imm < UINT8_MAX ? 8 : imm < UINT16_MAX ? 16 : imm < UINT32_MAX ? 32 : 64;
}

struct code *addrop(struct code *code)
{
    if (code->op1->type == CODE_ADDR) return code->op1;
    if (code->op2->type == CODE_ADDR) return code->op2;
    return NULL;
}

uint8_t opcode_arith2(struct code *code)
{
    uint8_t opcode = code->inst << 3;

    if (code->op2->type == CODE_IMM)
    {
        opcode |= 0x80;
        if (code->op2->size == 8 && code->size != 8) opcode |= 3;
    }
    else if (code->op2->type == CODE_ADDR)
        opcode |= (1 << 1);
    
    if (code->size != 8)
        opcode |= 1;

    return opcode;
}

void emit_rex(struct code *code, struct modrm *modrm, struct sib *sib)
{
    uint8_t rex = 0b01000000;
    if (code->size == 64) rex |= (1 << 3);

    rex |= !!(modrm->reg & 0b1000) << 2;
    rex |= !!(sib->index & 0b1000) << 1;

    if (sib->used) rex |= !!(sib->base & 0b1000);
    else rex |= !!(modrm->rm & 0b1000);

    if (rex != 0b01000000 || UNIBREG(code->op1) || UNIBREG(code->op2))
        emitb(rex);
}

void encode_addr(struct code *addr, struct modrm *modrm, struct sib *sib)
{
    if (addr->addr.isdisp)
    {
        if (addr->addr.base != -1)
            modrm->mod = immsize(addr->addr.disp) == 8 ? 1 : 2;
        else
            sib->base = 0b101;
    }

    if (addr->addr.index == -1 && addr->addr.base != -1 && addr->addr.base != REG_SP)
        modrm->rm = addr->addr.base;
    else
    {
        modrm->rm = 0b100;

        uint8_t s = addr->addr.scale;
        sib->scale = s == 8 ? 3 
                  : s == 4 ? 2
                  : s == 2 ? 1
                  : 0; // TODO: error if invalid scale

        sib->index = addr->addr.index == -1 ? 0b100 : addr->addr.index;
        sib->base = addr->addr.base == -1 ? 0b101 : addr->addr.base;
        sib->used = 1;
    }
}

#define REG(c) (c->type == CODE_REG)
#define IMM(c) (c->type == CODE_IMM)
#define ADDR(c) (c->type == CODE_ADDR)

// Instructions with 2 operands
void do_inst2(struct code *code)
{
    struct modrm modrm = { .used = 1 };
    struct sib sib = { 0 };

    struct code *addr = addrop(code);
    
    if (addr) encode_addr(addr, &modrm, &sib);
    else modrm.mod = 3;

    uint8_t opcode = 0;

    switch (code->inst)
    {
        case INST_ADD:
        case INST_OR:
        case INST_ADC:
        case INST_SBB:
        case INST_AND:
        case INST_SUB:
        case INST_XOR:
        case INST_CMP:
            opcode = opcode_arith2(code);

            if (IMM(code->op2))
                modrm.reg = code->inst;
            else if (REG(code->op2))
                modrm.reg = code->op2->reg;
            else if (REG(code->op1))
                modrm.reg = code->op1->reg;
            
            if (REG(code->op1) && REG(code->op2))
                modrm.rm = code->op1->reg;

            break;

        case INST_MOV:
            if (IMM(code->op2))
            {
                modrm.used = !!addr;
                code->op2->size = code->size;
                if (REG(code->op1))
                    opcode = (code->size == 8 ? 0xb0 : 0xb8) + code->op1->reg;
                else
                {
                    if (code->op2->size == 64) code->op2->size = 32;
                    opcode = 0xc6 | (code->size != 8);
                }
            }
            else
                opcode = 0x88 | (ADDR(code->op2) << 1) | (code->size != 8);

            if (REG(code->op2))
                modrm.reg = code->op2->reg;
            else if (REG(code->op1))
                modrm.reg = code->op1->reg;

            if (REG(code->op1) && REG(code->op2))
                modrm.rm = code->op1->reg;

            break;
    }

    if (code->size == 16)
        emitb(0x66);
 
    if (addr && addr->size == 32)
        emitb(0x67);

    emit_rex(code, &modrm, &sib);
    emitb(opcode);

    if (modrm.used)
        emitb(modrm.mod << 6 | modrm.reg << 3 | modrm.rm);

    if (sib.used)
        emitb(sib.scale << 6 | sib.index << 3 | sib.base);

    if (addr && addr->addr.isdisp)
        emit(addr->addr.disp, modrm.mod == 1 ? 8 : addr->size);

    if (IMM(code->op2))
        emit(code->op2->imm, code->op2->size);
}

void do_inst(struct code *code)
{
    if (code->op1 && code->op2) do_inst2(code);
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

            code->op1 = parse();
            expect(T_COMMA);
            code->op2 = parse();
            if (!code->size)
            {
                if (REG(code->op1)) code->size = code->op1->size;
                else if (REG(code->op2)) code->size = code->op2->size;
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
            code->size = immsize(code->imm);
            s_t++;
            break;

        case T_LBRACK: parse_addr(code); break;
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
