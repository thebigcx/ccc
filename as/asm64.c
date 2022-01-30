#include "asm.h"
#include "decl.h"
#include "inst.h"
#include "lib.h"
#include "elf.h"
#include "parse.h"

#include <string.h>
#include <stdio.h>
#include <stddef.h>
#include <ctype.h>
#include <stdlib.h>

struct inst g_insttbl[] =
{
    // Arithmetic
    { .mnem = "add", .opcode = 0x00, .op1 = OP_REG | OP_SIZE8, .op2 = OP_RM | OP_SIZE8, .reg = -1 },
    { .mnem = "add", .opcode = 0x01, .op1 = OP_REG | OP_SZEX8, .op2 = OP_RM | OP_SZEX8, .reg = -1 },
    { .mnem = "add", .opcode = 0x02, .op1 = OP_RM | OP_SIZE8, .op2 = OP_REG | OP_SIZE8, .reg = -1 },
    { .mnem = "add", .opcode = 0x03, .op1 = OP_RM | OP_SZEX8, .op2 = OP_REG | OP_SZEX8, .reg = -1 },
    { .mnem = "add", .opcode = 0x80, .op1 = OP_IMM | OP_SIZE8, .op2 = OP_RM | OP_SIZE8, .reg = 0  },
    { .mnem = "add", .opcode = 0x83, .op1 = OP_IMM | OP_SIZE8, .op2 = OP_RM | OP_SZEX8, .reg = 0  },
    { .mnem = "add", .opcode = 0x81, .op1 = OP_IMM | OP_SIZE16 | OP_SIZE32, .op2 = OP_RM | OP_SZEX8, .reg = 0  },

    { .mnem = "or", .opcode = 0x08, .op1 = OP_REG | OP_SIZE8, .op2 = OP_RM | OP_SIZE8, .reg = -1 },
    { .mnem = "or", .opcode = 0x09, .op1 = OP_REG | OP_SZEX8, .op2 = OP_RM | OP_SZEX8, .reg = -1 },
    { .mnem = "or", .opcode = 0x0a, .op1 = OP_RM | OP_SIZE8, .op2 = OP_REG | OP_SIZE8, .reg = -1 },
    { .mnem = "or", .opcode = 0x0b, .op1 = OP_RM | OP_SZEX8, .op2 = OP_REG | OP_SZEX8, .reg = -1 },
    { .mnem = "or", .opcode = 0x80, .op1 = OP_IMM | OP_SIZE8, .op2 = OP_RM | OP_SIZE8, .reg = 1  },
    { .mnem = "or", .opcode = 0x83, .op1 = OP_IMM | OP_SIZE8, .op2 = OP_RM | OP_SZEX8, .reg = 1  },
    { .mnem = "or", .opcode = 0x81, .op1 = OP_IMM | OP_SIZE16 | OP_SIZE32, .op2 = OP_RM | OP_SZEX8, .reg = 1  },
    
    { .mnem = "and", .opcode = 0x20, .op1 = OP_REG | OP_SIZE8, .op2 = OP_RM | OP_SIZE8, .reg = -1 },
    { .mnem = "and", .opcode = 0x21, .op1 = OP_REG | OP_SZEX8, .op2 = OP_RM | OP_SZEX8, .reg = -1 },
    { .mnem = "and", .opcode = 0x22, .op1 = OP_RM | OP_SIZE8, .op2 = OP_REG | OP_SIZE8, .reg = -1 },
    { .mnem = "and", .opcode = 0x23, .op1 = OP_RM | OP_SZEX8, .op2 = OP_REG | OP_SZEX8, .reg = -1 },
    { .mnem = "and", .opcode = 0x80, .op1 = OP_IMM | OP_SIZE8, .op2 = OP_RM | OP_SIZE8, .reg = 4  },
    { .mnem = "and", .opcode = 0x83, .op1 = OP_IMM | OP_SIZE8, .op2 = OP_RM | OP_SZEX8, .reg = 4  },
    { .mnem = "and", .opcode = 0x81, .op1 = OP_IMM | OP_SIZE16 | OP_SIZE32, .op2 = OP_RM | OP_SZEX8, .reg = 4  },

    { .mnem = "sub", .opcode = 0x28, .op1 = OP_REG | OP_SIZE8, .op2 = OP_RM | OP_SIZE8, .reg = -1 },
    { .mnem = "sub", .opcode = 0x29, .op1 = OP_REG | OP_SZEX8, .op2 = OP_RM | OP_SZEX8, .reg = -1 },
    { .mnem = "sub", .opcode = 0x2a, .op1 = OP_RM | OP_SIZE8, .op2 = OP_REG | OP_SIZE8, .reg = -1 },
    { .mnem = "sub", .opcode = 0x2b, .op1 = OP_RM | OP_SZEX8, .op2 = OP_REG | OP_SZEX8, .reg = -1 },
    { .mnem = "sub", .opcode = 0x80, .op1 = OP_IMM | OP_SIZE8, .op2 = OP_RM | OP_SIZE8, .reg = 5  },
    { .mnem = "sub", .opcode = 0x83, .op1 = OP_IMM | OP_SIZE8, .op2 = OP_RM | OP_SZEX8, .reg = 5  },
    { .mnem = "sub", .opcode = 0x81, .op1 = OP_IMM | OP_SIZE16 | OP_SIZE32, .op2 = OP_RM | OP_SZEX8, .reg = 5  },

    { .mnem = "xor", .opcode = 0x30, .op1 = OP_REG | OP_SIZE8, .op2 = OP_RM | OP_SIZE8, .reg = -1 },
    { .mnem = "xor", .opcode = 0x31, .op1 = OP_REG | OP_SZEX8, .op2 = OP_RM | OP_SZEX8, .reg = -1 },
    { .mnem = "xor", .opcode = 0x32, .op1 = OP_RM | OP_SIZE8, .op2 = OP_REG | OP_SIZE8, .reg = -1 },
    { .mnem = "xor", .opcode = 0x33, .op1 = OP_RM | OP_SZEX8, .op2 = OP_REG | OP_SZEX8, .reg = -1 },
    { .mnem = "xor", .opcode = 0x80, .op1 = OP_IMM | OP_SIZE8, .op2 = OP_RM | OP_SIZE8, .reg = 6  },
    { .mnem = "xor", .opcode = 0x83, .op1 = OP_IMM | OP_SIZE8, .op2 = OP_RM | OP_SZEX8, .reg = 6  },
    { .mnem = "xor", .opcode = 0x81, .op1 = OP_IMM | OP_SIZE16 | OP_SIZE32, .op2 = OP_RM | OP_SZEX8, .reg = 6  },

    { .mnem = "cmp", .opcode = 0x38, .op1 = OP_REG | OP_SIZE8, .op2 = OP_RM | OP_SIZE8, .reg = -1 },
    { .mnem = "cmp", .opcode = 0x39, .op1 = OP_REG | OP_SZEX8, .op2 = OP_RM | OP_SZEX8, .reg = -1 },
    { .mnem = "cmp", .opcode = 0x3a, .op1 = OP_RM | OP_SIZE8, .op2 = OP_REG | OP_SIZE8, .reg = -1 },
    { .mnem = "cmp", .opcode = 0x3b, .op1 = OP_RM | OP_SZEX8, .op2 = OP_REG | OP_SZEX8, .reg = -1 },
    { .mnem = "cmp", .opcode = 0x80, .op1 = OP_IMM | OP_SIZE8, .op2 = OP_RM | OP_SIZE8, .reg = 7  },
    { .mnem = "cmp", .opcode = 0x83, .op1 = OP_IMM | OP_SIZE8, .op2 = OP_RM | OP_SZEX8, .reg = 7  },
    { .mnem = "cmp", .opcode = 0x81, .op1 = OP_IMM | OP_SIZE16 | OP_SIZE32, .op2 = OP_RM | OP_SZEX8, .reg = 7  },
    
    { .mnem = "test", .opcode = 0x84, .op1 = OP_REG | OP_SIZE8, .op2 = OP_RM | OP_SIZE8, .reg = -1 },
    { .mnem = "test", .opcode = 0x85, .op1 = OP_REG | OP_SZEX8, .op2 = OP_RM | OP_SZEX8, .reg = -1 },
    { .mnem = "test", .opcode = 0xf6, .op1 = OP_IMM | OP_SIZE8, .op2 = OP_RM | OP_SIZE8, .reg = 0 },
    { .mnem = "test", .opcode = 0xf7, .op1 = OP_IMM | OP_SIZE16 | OP_SIZE32, .op2 = OP_RM | OP_SZEX8, .reg = 0 },

    { .mnem = "mul", .opcode = 0xf6, .op1 = OP_RM | OP_SIZE8, .reg = 4 },
    { .mnem = "mul", .opcode = 0xf7, .op1 = OP_RM | OP_SZEX8, .reg = 4 },
    { .mnem = "imul", .opcode = 0xf6, .op1 = OP_RM | OP_SIZE8, .reg = 5 },
    { .mnem = "imul", .opcode = 0xf7, .op1 = OP_RM | OP_SZEX8, .reg = 5 },
    { .mnem = "imul", .pre = 0x0f, .opcode = 0xaf, .op1 = OP_RM | OP_SZEX8, .op2 = OP_REG | OP_SZEX8, .reg = -1  },

    { .mnem = "div", .opcode = 0xf6, .op1 = OP_RM | OP_SIZE8, .reg = 6 },
    { .mnem = "div", .opcode = 0xf7, .op1 = OP_RM | OP_SZEX8, .reg = 6 },
    { .mnem = "idiv", .opcode = 0xf6, .op1 = OP_RM | OP_SIZE8, .reg = 7 },
    { .mnem = "idiv", .opcode = 0xf7, .op1 = OP_RM | OP_SZEX8, .reg = 7 },

    { .mnem = "inc", .opcode = 0xfe, .op1 = OP_RM | OP_SIZE8, .reg = 0 },
    { .mnem = "inc", .opcode = 0xff, .op1 = OP_RM | OP_SZEX8, .reg = 0 },

    { .mnem = "dec", .opcode = 0xfe, .op1 = OP_RM | OP_SIZE8, .reg = 1 },
    { .mnem = "dec", .opcode = 0xff, .op1 = OP_RM | OP_SZEX8, .reg = 1 },

    { .mnem = "not", .opcode = 0xf6, .op1 = OP_RM | OP_SIZE8, .reg = 2 },
    { .mnem = "not", .opcode = 0xf7, .op1 = OP_RM | OP_SZEX8, .reg = 2 },
    { .mnem = "neg", .opcode = 0xf6, .op1 = OP_RM | OP_SIZE8, .reg = 3 },
    { .mnem = "neg", .opcode = 0xf7, .op1 = OP_RM | OP_SZEX8, .reg = 3 },

    // Shift
    { .mnem = "shl", .opcode = 0xc0, .op1 = OP_IMM | OP_SIZE8, .op2 = OP_RM | OP_SIZE8, .reg = 4  },
    { .mnem = "shl", .opcode = 0xc1, .op1 = OP_IMM | OP_SIZE8, .op2 = OP_RM | OP_SZEX8, .reg = 4  },
    { .mnem = "shr", .opcode = 0xc0, .op1 = OP_IMM | OP_SIZE8, .op2 = OP_RM | OP_SIZE8, .reg = 7  },
    { .mnem = "shr", .opcode = 0xc1, .op1 = OP_IMM | OP_SIZE8, .op2 = OP_RM | OP_SZEX8, .reg = 7  },
    
    // Moves
    { .mnem = "mov", .opcode = 0x88, .op1 = OP_REG | OP_SIZE8, .op2 = OP_RM | OP_SIZE8, .reg = -1  },
    { .mnem = "mov", .opcode = 0x89, .op1 = OP_REG | OP_SZEX8, .op2 = OP_RM | OP_SZEX8, .reg = -1  },
    { .mnem = "mov", .opcode = 0x8a, .op1 = OP_RM | OP_SIZE8, .op2 = OP_REG | OP_SIZE8, .reg = -1  },
    { .mnem = "mov", .opcode = 0x8b, .op1 = OP_RM | OP_SZEX8, .op2 = OP_REG | OP_SZEX8, .reg = -1  },

    { .mnem = "mov", .opcode = 0xc6, .op1 = OP_IMM | OP_SIZE8, .op2 = OP_RM | OP_SIZE8, .reg = 0 },
    { .mnem = "mov", .opcode = 0xc7, .op1 = OP_IMM | OP_SIZE32, .op2 = OP_RM | OP_SIZE64, .reg = 0 },
    { .mnem = "mov", .opcode = 0xb0, .op1 = OP_IMM | OP_SIZE8, .op2 = OP_REG | OP_SIZE8, .reg = -1, .flags = IF_ROPCODE  },
    { .mnem = "mov", .opcode = 0xb8, .op1 = OP_IMM | OP_SIZE64, .op2 = OP_REG | OP_SIZE64, .reg = -1, .flags = IF_ROPCODE  },

    { .mnem = "movzx", .pre = 0x0f, .opcode = 0xb6, .op1 = OP_RM | OP_SIZE8,  .op2 = OP_REG | OP_SZEX8, .reg = -1 },
    { .mnem = "movzx", .pre = 0x0f, .opcode = 0xb7, .op1 = OP_RM | OP_SIZE16, .op2 = OP_REG | OP_SZEX8, .reg = -1 },
    { .mnem = "movsx", .pre = 0x0f, .opcode = 0xbe, .op1 = OP_RM | OP_SIZE8,  .op2 = OP_REG | OP_SZEX8, .reg = -1 },
    { .mnem = "movsx", .pre = 0x0f, .opcode = 0xbf, .op1 = OP_RM | OP_SIZE16, .op2 = OP_REG | OP_SZEX8, .reg = -1 },
    { .mnem = "movsx", .opcode = 0x63, .op1 = OP_RM | OP_SIZE32, .op2 = OP_REG | OP_SZEX8, .reg = -1 },

    // Jump & Conditional jumps
    { .mnem = "jmp", .opcode = 0xe9, .op1 = OP_IMM | OP_SIZE32, .reg = -1, .flags = IF_REL },
    { .mnem = "jz",  .pre = 0x0f, .opcode = 0x84, .op1 = OP_IMM | OP_SIZE32, .reg = -1, .flags = IF_REL },
    { .mnem = "jnz", .pre = 0x0f, .opcode = 0x85, .op1 = OP_IMM | OP_SIZE32, .reg = -1, .flags = IF_REL },
    { .mnem = "js",  .pre = 0x0f, .opcode = 0x88, .op1 = OP_IMM | OP_SIZE32, .reg = -1, .flags = IF_REL },
    { .mnem = "jl",  .pre = 0x0f, .opcode = 0x8c, .op1 = OP_IMM | OP_SIZE32, .reg = -1, .flags = IF_REL },
    { .mnem = "jge", .pre = 0x0f, .opcode = 0x8d, .op1 = OP_IMM | OP_SIZE32, .reg = -1, .flags = IF_REL },
    { .mnem = "jle", .pre = 0x0f, .opcode = 0x8e, .op1 = OP_IMM | OP_SIZE32, .reg = -1, .flags = IF_REL },
    { .mnem = "jg",  .pre = 0x0f, .opcode = 0x8f, .op1 = OP_IMM | OP_SIZE32, .reg = -1, .flags = IF_REL },

    // Set on condition
    { .mnem = "setz",  .pre = 0x0f, .opcode = 0x94, .op1 = OP_RM | OP_SIZE8, .reg = 0 },
    { .mnem = "setnz", .pre = 0x0f, .opcode = 0x95, .op1 = OP_RM | OP_SIZE8, .reg = 0 },
    { .mnem = "sets",  .pre = 0x0f, .opcode = 0x98, .op1 = OP_RM | OP_SIZE8, .reg = 0 },
    { .mnem = "setl",  .pre = 0x0f, .opcode = 0x9c, .op1 = OP_RM | OP_SIZE8, .reg = 0 },
    { .mnem = "setge", .pre = 0x0f, .opcode = 0x9d, .op1 = OP_RM | OP_SIZE8, .reg = 0 },
    { .mnem = "setle", .pre = 0x0f, .opcode = 0x9e, .op1 = OP_RM | OP_SIZE8, .reg = 0 },
    { .mnem = "setg",  .pre = 0x0f, .opcode = 0x9f, .op1 = OP_RM | OP_SIZE8, .reg = 0 },

    // Misc.
    { .mnem = "ret", .opcode = 0xc3, .reg = -1  },
    { .mnem = "syscall", .pre = 0x0f, .opcode = 0x05, .reg = -1  },
    { .mnem = "int", .opcode = 0xcd, .op1 = OP_IMM | OP_SIZE8, .reg = -1 },
    { .mnem = "push", .opcode = 0x50, .op1 = OP_REG | OP_SIZE16 | OP_SIZE64, .reg = -1, .flags = IF_ROPCODE | IF_DEF64 },
    { .mnem = "pop", .opcode = 0x58, .op1 = OP_REG | OP_SIZE16 | OP_SIZE64, .reg = -1, .flags = IF_ROPCODE | IF_DEF64 },
    { .mnem = "call", .opcode = 0xe8, .op1 = OP_IMM | OP_SIZE32, .reg = -1, .flags = IF_REL },
    { .mnem = "lea", .opcode = 0x8d, .op1 = OP_MEM | OP_SIZEM, .op2 = OP_REG | OP_SIZE16 | OP_SIZE32 | OP_SIZE64, .reg = -1 },
    { .mnem = "leave", .opcode = 0xc9, .reg = -1 },
   
    { .mnem = "cwd", .opcode = 0x99, .reg = -1, .size = OP_SIZE16 },
    { .mnem = "cdq", .opcode = 0x99, .reg = -1, .size = OP_SIZE32 },
    { .mnem = "cqo", .opcode = 0x99, .reg = -1, .size = OP_SIZE64 },
};

struct inst *searchi(struct code *code)
{
    for (size_t i = 0; i < ARRLEN(g_insttbl); i++)
    {
        int op1u = code->op1.type & OP_TYPEM;
        int op2u = code->op2.type & OP_TYPEM;
        if (!strcmp(g_insttbl[i].mnem, code->mnem)
                && (!op1u || (code->op1.type & OP_TYPEM) & (g_insttbl[i].op1 & OP_TYPEM))
                && (!op1u || (code->op1.type & OP_SIZEM) & (g_insttbl[i].op1 & OP_SIZEM))
                && (!op2u || (code->op2.type & OP_TYPEM) & (g_insttbl[i].op2 & OP_TYPEM))
                && (!op2u || (code->op2.type & OP_SIZEM) & (g_insttbl[i].op2 & OP_SIZEM)))
            return &g_insttbl[i];
    }

    return NULL;
}

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

#define REXFIX (0b01000000) // Fixed REX bit pattern

uint8_t mkrex(int is64, struct modrm *modrm)
{
    return REXFIX | (!!is64 << 3) | (!!(modrm->reg & 0b1000) << 2) | !!(modrm->rm & 0b1000);
}

int iscode64(struct code *code, struct inst *inst)
{
    return inst->size & OP_SIZE64 || (!(inst->flags & IF_DEF64) && (ISREGSZ(code->op1.type, OP_SIZE64) || ISREGSZ(code->op2.type, OP_SIZE64)));
}

size_t immsize(uint64_t imm)
{
    return imm < UINT8_MAX ? 1 : imm < UINT16_MAX ? 2 : imm < UINT32_MAX ? 4 : 8;
}

void mkmodrmsib(struct modrm *modrm, struct sib *sib, struct code *code, struct inst *inst)
{
    struct codeop *mem = ISMEM(inst->op1) && ISMEM(code->op1.type) ? &code->op1
                       : ISMEM(inst->op2) && ISMEM(code->op2.type) ? &code->op2 : NULL;

    struct codeop *reg = (inst->op1 & OP_TYPEM) == OP_REG && ISREG(code->op1.type) ? &code->op1
                       : (inst->op2 & OP_TYPEM) == OP_REG && ISREG(code->op2.type) ? &code->op2 : NULL;

    struct codeop *rm = (inst->op1 & OP_TYPEM) == OP_RM && ISRM(code->op1.type) ? &code->op1
                      : (inst->op2 & OP_TYPEM) == OP_RM && ISRM(code->op2.type) ? &code->op2 : NULL;

    if (!reg && !rm) return;

    if (!modrm->reg && reg && reg != rm) modrm->reg = reg->val;
    if (!mem)
    {
        modrm->mod = 0b11;
        if (rm) modrm->rm = rm->val;
    }
    else
    {
        if (mem->sib.base == REG_RIP)
        {
            modrm->mod = 0;
            modrm->rm = 0b101;
            return;
        }

        if (!(mem->sib.flags & SIB_NODISP))
        {
            if (immsize(mem->val) == 1)
                mem->sib.flags |= SIB_DISP8;
            if (mem->sib.base != (uint8_t)-1)
                modrm->mod = immsize(mem->val) == 1 ? 1 : 2;
            else
                sib->base = 0b101;
        }

        if (mem->sib.idx == (uint8_t)-1 && mem->sib.base != (uint8_t)-1 && mem->sib.base != REG_SP)
            modrm->rm = mem->sib.base;
        else
        {
            modrm->rm = 0b100;

            sib->scale = mem->sib.scale == 8 ? 3
                       : mem->sib.scale == 4 ? 2
                       : mem->sib.scale == 2 ? 1 : 0;

            sib->idx = mem->sib.idx == (uint8_t)-1 ? 0b100 : mem->sib.idx;
            sib->base = mem->sib.base == (uint8_t)-1 ? 0b101 : mem->sib.base;
            sib->flags |= SIB_USED;
        }
    }
}

// Operand-size override prefix
#define OPOVPRE(code, inst) ((inst).size & OP_SIZE16 || ISREGSZ((code).op1.type, OP_SIZE16) || ISREGSZ((code).op2.type, OP_SIZE16))
#define ADROVPRE(code, inst) ((ISMEM((code).op1.type) && (code).op1.sib.flags & SIB_32BIT) || (ISMEM((code).op2.type) && (code).op2.sib.flags & SIB_32BIT))

size_t instsize64(struct inst *inst, struct code *code)
{
    size_t s = 1; // opcode

    struct modrm modrm = { .reg = inst->reg != (uint8_t)-1 ? inst->reg : 0 };
    struct sib sib = { 0 };
    mkmodrmsib(&modrm, &sib, code, inst);
    
    uint8_t rex = mkrex(iscode64(code, inst), &modrm);
    if (rex != REXFIX) s++;

    if (OPOVPRE(*code, *inst)) s++;
    if (ADROVPRE(*code, *inst)) s++;

    if (inst->pre) s++;

    if ((modrm.reg || ISRM(inst->op1) || ISRM(inst->op2)) && !(inst->flags & IF_ROPCODE))
        s++;

    if (sib.flags & SIB_USED)
        s++;

    if (ISMEM(code->op1.type) && !(code->op1.sib.flags & SIB_NODISP))
        s += code->op1.sib.flags & SIB_DISP8 ? 1 : 4;
    else if (ISMEM(code->op2.type) && !(code->op2.sib.flags & SIB_NODISP))
        s += code->op2.sib.flags & SIB_DISP8 ? 1 : 4;

    if (ISIMM(code->op1.type))
        s += (inst->op1 & OP_SIZEM) >> 3;

    return s;
}

void assemble64(struct code *code, struct inst *inst, size_t lc)
{
    struct modrm modrm = { .reg = inst->reg != (uint8_t)-1 ? inst->reg : 0 };
    struct sib sib = { 0 };
    mkmodrmsib(&modrm, &sib, code, inst);
    
    uint8_t rex = mkrex(iscode64(code, inst), &modrm);
    if (rex != REXFIX) emit8(rex);

    if (OPOVPRE(*code, *inst)) emit8(0x66);
    if (ADROVPRE(*code, *inst)) emit8(0x67);

    if (inst->pre) emit8(inst->pre);

    uint8_t opcode = inst->opcode;

    if (inst->flags & IF_ROPCODE) opcode += modrm.reg & 0b111;

    emit8(opcode);

    if ((modrm.reg || ISRM(inst->op1) || ISRM(inst->op2)) && !(inst->flags & IF_ROPCODE))
        emit8((modrm.mod << 6) | ((modrm.reg & 0b111) << 3) | (modrm.rm & 0b111));

    if (sib.flags & SIB_USED)
        emit8((sib.scale << 6) | (sib.idx << 3) | sib.base);

    if (ISMEM(code->op1.type) && !(code->op1.sib.flags & SIB_NODISP))
    {
        if (code->op1.sym)
        {
            struct symbol *sym = findsym(code->op1.sym);
            if (code->op1.sib.base == REG_RIP)
            {
                sect_add_reloc(g_currsect, ftell(g_outf) - g_currsect->offset, sym, sym->val - 4, REL_PCREL);
                code->op1.val = 0;
            }
            else
                code->op1.val = sym->val;
        }
        
        emit(code->op1.sib.flags & SIB_DISP8 ? OP_SIZE8 : OP_SIZE32, code->op1.val);
    }
    else if (ISMEM(code->op2.type) && !(code->op2.sib.flags & SIB_NODISP))
        emit(code->op2.sib.flags & SIB_DISP8 ? OP_SIZE8 : OP_SIZE32, code->op2.val);

    if (ISIMM(code->op1.type))
    {
        struct symbol *sym = NULL;
        if (code->op1.sym)
        {
            if (!strcmp(code->op1.sym, "."))
            {
                sym = findsym(g_currsect->name);
            }
            else
            {
                sym = findsym(code->op1.sym);
                if (!sym)
                {
                    struct symbol null = {
                        .flags = SYM_UNDEF | SYM_GLOB,
                        .name = strdup(code->op1.sym)
                    };
                    addsym(&null);
                    sym = findsym(code->op1.sym);
                }
                if (sym->flags & SYM_UNDEF)
                {
                    sect_add_reloc(g_currsect, ftell(g_outf) - g_currsect->offset, sym, -4, 0);
                    code->op1.val = 0;

                    emit(inst->op1 & OP_SIZEM, code->op1.val);
                    return;
                }
            }
        }
        
        if (inst->flags & IF_REL)
        {
            if (code->op1.sym) code->op1.val += sym->val;
            code->op1.val -= lc;
        }
        else if (code->op1.sym)
        {
            sect_add_reloc(g_currsect, ftell(g_outf) - g_currsect->offset, sym, sym->val, 0);
            code->op1.val = 0;
        }

        emit(inst->op1 & OP_SIZEM, code->op1.val);
    }
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

            free(direct);
            continue;
        }

        struct code code = parse_code(strt);
        struct inst *ent = searchi(&code);
        if (!ent)
            error("Invalid instruction\n");

        lc += instsize64(ent, &code);

        assemble64(&code, ent, lc);
    }

    if (g_currsect) g_currsect->size = ftell(g_outf) - g_currsect->offset;
    elf_end_file();
}
