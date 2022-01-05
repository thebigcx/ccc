#pragma once

#include <stddef.h>
#include <stdio.h>

#define CODE_INST 0 // Instruction
#define CODE_LBL  1 // Label
#define CODE_REG  2 // Register
#define CODE_ADDR 3 // Address or displacement

struct code
{
    int type;
    struct code *op1, *op2;
    size_t size; // 8, 16, 32, 64

    union
    {
        int inst;
        int reg;
        char *sval;
    };
};

struct token;

int assemble(FILE *out, struct token *toks);
