#pragma once

#include <stddef.h>

#define CODE_INST 0 // Instruction
#define CODE_LBL  1 // Label
#define CODE_REG  2 // Register
#define CODE_ADDR 3 // Address or displacement
#define CODE_IMM  4 // Immediate

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

int parse_file(struct code ***code, size_t *codecnt, struct token *toks);
