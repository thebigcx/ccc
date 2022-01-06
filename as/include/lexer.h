#pragma once

#include <stddef.h>
#include <stdio.h>

enum TOKEN
{
    T_LBL,
    T_REG,
    T_IMM,
    T_COLON,
    T_COMMA,
    T_INST,
    T_LBRACK,
    T_RBRACK,
    T_LPAREN,
    T_RPAREN,
    T_PLUS,
    T_STAR,
    T_EOF
};

struct token
{
    int type;
    char *sval;
    unsigned long ival;
};

int lexfile(FILE *file, struct token **toks);
