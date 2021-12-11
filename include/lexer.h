#pragma once

enum TOKTYPE
{
    T_EOF,
    T_ID,
    T_PLUS,
    T_MINUS,
    T_STAR,
    T_SLASH,
    T_INTLIT, T_STRLIT,
    T_SEMI,
    T_COMMA,
    T_AMP,
    T_COLON,

    T_IDENT,

    T_EQ, T_EQEQ, T_NEQ, T_GT, T_LT, T_GTE, T_LTE, T_NOT,

    T_INT8, T_INT16, T_INT32, T_INT64,
    T_UINT8, T_UINT16, T_UINT32, T_UINT64,
    T_FLOAT32, T_FLOAT64,

    T_LPAREN, T_RPAREN, T_LBRACK, T_RBRACK, T_LBRACE, T_RBRACE,

    T_ASM,

    T_RETURN, T_WHILE, T_IF, T_ELSE, T_FOR, T_FUNC, T_VAR, T_SIZEOF
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
    int line, col;
};

int tokenize(const char *str, struct token **toks);
