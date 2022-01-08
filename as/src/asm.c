#include <asm.h>
#include <defs.h>
#include <lexer.h>
#include <parse.h>

#include <stdarg.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <elf.h>

#define ASSERT_REGMATCH(r1, r2) if (regsize(r1) != regsize(r2)) error("Mismatched register sizes.\n")

static FILE *s_out = NULL;

// TODO: do error checking in parser
extern void error(const char *format, ...);

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
#define UNIBREG(c) (c && c->type == CODE_REG && c->size == 8 && (c->reg == REG_SP || c->reg == REG_BP || c->reg == REG_SI || c->reg == REG_DI))

int immsize(unsigned long imm)
{
    return imm < UINT8_MAX ? 8 : imm < UINT16_MAX ? 16 : imm < UINT32_MAX ? 32 : 64;
}

struct code *addrop(struct code *code)
{
    if (code->op1 && code->op1->type == CODE_ADDR) return code->op1;
    if (code->op2 && code->op2->type == CODE_ADDR) return code->op2;
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

// Defaults to 64-bit
#define DEF64(c) (c == INST_PUSH || c == INST_POP)

void emit_rex(struct code *code, struct modrm *modrm, struct sib *sib)
{
    uint8_t rex = 0b01000000;
    if (code->size == 64 && !DEF64(code->inst)) rex |= (1 << 3);

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
void do_inst(struct code *code)
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

        case INST_PUSH:
            if (IMM(code->op1))
            {
                opcode = 0x68 | ((code->size == 8) << 1);
                modrm.used = 0;
            }
            else if (REG(code->op1))
            {
                if (code->op1->size == 32) error("Invalid size for PUSH instruction.\n");
                opcode = 0x50 + code->op1->reg;
                modrm.used = 0;
            }
            else
            {
                opcode = 0xff;
                modrm.reg = 6;
            }

            break;

        case INST_POP:
            if (code->size == 32) error("Invalid size for POP instruction.\n");
            if (REG(code->op1))
            {
                opcode = 0x58 + code->op1->reg;
                modrm.used = 0;
            }
            else
                opcode = 0x8f;
        
            break; 

        case INST_RET:
            opcode = 0xc3;
            modrm.used = 0;
            break;
    
        case INST_HLT:
            opcode = 0xf4;
            modrm.used = 0;
            break;

        case INST_CLI:
            opcode = 0xfa;
            modrm.used = 0;
            break;
    
        case INST_STI:
            opcode = 0xfb;
            modrm.used = 0;
            break;

/*        case INST_JMP:
            if (IMM(code->op1))
            {
                opcode = 0xe9 | ((code->op1->size == 8) << 1);
                modrm.used = 0;
                code->op1->imm -= ftell(s_out);
            }

            break;*/
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

    if (code->op2 && IMM(code->op2))
        emit(code->op2->imm, code->op2->size);
    else if (code->op1 && IMM(code->op1))
        emit(code->op1->imm, code->op1->size);
}

int assemble(FILE *out, struct code **code, size_t codecnt)
{
    s_out = out;

    for (unsigned int i = 0; i < codecnt; i++)
    {
        switch (code[i]->type)
        {
            case CODE_INST: do_inst(code[i]); break;
        }
    }

    return 0;
}

/*int assemble(FILE *out, struct token *toks)
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
}*/
