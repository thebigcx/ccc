#pragma once

#include <stddef.h>
#include <stdio.h>

// Lexer register names
enum REG
{ 
    LREG_AH,
    LREG_BH,
    LREG_CH,
    LREG_DH,

    LREG_AL,
    LREG_BL,
    LREG_CL,
    LREG_DL,
    LREG_SPL,
    LREG_BPL,
    LREG_SIL,
    LREG_DIL,

    LREG_AX,
    LREG_BX,
    LREG_CX,
    LREG_DX,
    LREG_SP,
    LREG_BP,
    LREG_SI,
    LREG_DI,

    LREG_EAX,
    LREG_EBX,
    LREG_ECX,
    LREG_EDX,
    LREG_ESP,
    LREG_EBP,
    LREG_ESI,
    LREG_EDI,

    LREG_RAX,
    LREG_RBX,
    LREG_RCX,
    LREG_RDX,
    LREG_RSP,
    LREG_RBP,
    LREG_RSI,
    LREG_RDI,
};

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
    T_U8,
    T_U16,
    T_U32,
    T_U64,
    T_EOF
};

struct token
{
    int type;
    char *sval;
    unsigned long ival;
};

int lexfile(FILE *file, struct token **toks);
