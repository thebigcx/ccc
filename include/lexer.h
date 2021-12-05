#pragma once

enum TOKTYPE
{
    T_EOF,
    T_ID,
    T_PLUS,
    T_MINUS,
    T_STAR,
    T_SLASH,
    T_INTLIT,
    T_SEMI,

    T_IDENT,

    T_INT8, T_INT16, T_INT32, T_INT64,
    T_UINT8, T_UINT16, T_UINT32, T_UINT64,
    T_FLOAT32, T_FLOAT64,

    T_LPAREN, T_RPAREN, T_LBRACK, T_RBRACK, T_LBRACE, T_RBRACE
};

// Type from 'enum TOKTYPE' and string or integer value
struct token
{
    int type;
    union
    {
        unsigned long ival;
        char *sval;
    } v;
};

int tokenize(const char *str, struct token **toks);
