#pragma once

#include <stddef.h>
#include <stdio.h>
#include <stdint.h>

#define CODE_INST 0 // Instruction
#define CODE_LBL  1 // Label
#define CODE_REG  2 // Register
#define CODE_ADDR 3 // Address or displacement
#define CODE_IMM  4 // Immediate

#define REG_AX  0b0000
#define REG_CX  0b0001
#define REG_DX  0b0010
#define REG_BX  0b0011
#define REG_SP  0b0100
#define REG_BP  0b0101
#define REG_SI  0b0110
#define REG_DI  0b0111
#define REG_R8  0b1000
#define REG_R9  0b1001
#define REG_R10 0b1010
#define REG_R11 0b1011
#define REG_R12 0b1100
#define REG_R13 0b1101
#define REG_R14 0b1110
#define REG_R15 0b1111

struct modrm
{
    uint8_t mod, reg, rm;
    int used;
};

struct sib
{
    uint8_t scale, index, base;
    int used;
};

struct code
{
    int type;
    struct code *op1, *op2;
    size_t size; // 8, 16, 32, 64

    union
    {
        int inst;
        int reg;
        unsigned long imm;
        char *sval;

        struct
        {
            int isdisp; // Is displaced address
            unsigned long disp, scale;
            int base, index;
        } addr; // Addressing
    };
};

struct token;

int assemble(FILE *out, struct token *toks);
