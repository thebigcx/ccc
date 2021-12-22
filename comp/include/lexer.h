#pragma once

enum TOKTYPE
{
    T_EOF,
    T_ID,
    T_PLUS,
    T_MINUS,
    T_STAR,
    T_SLASH,
    T_PLUSEQ,
    T_MINUSEQ,
    T_MULEQ,
    T_DIVEQ,
    T_XOREQ,
    T_ANDEQ,
    T_OREQ,
    T_SHLEQ,
    T_SHREQ,
    T_SHL,
    T_SHR,
    T_INTLIT, T_STRLIT,
    T_SEMI,
    T_COMMA,
    T_AMP,
    T_COLON,
    T_DOT,
    T_ARROW,
    T_ELLIPSIS,

    T_IDENT,

    T_EQ, T_EQEQ, T_NEQ, T_GT, T_LT, T_GTE, T_LTE, T_NOT, T_LAND, T_LOR, T_BITOR, T_BITXOR, T_BITNOT, T_TERNARY,

    T_INC, T_DEC,

    T_INT8, T_INT16, T_INT32, T_INT64,
    T_UINT8, T_UINT16, T_UINT32, T_UINT64,
    T_FLOAT32, T_FLOAT64,

    T_LPAREN, T_RPAREN, T_LBRACK, T_RBRACK, T_LBRACE, T_RBRACE,

    T_ASM,

    T_RETURN, T_WHILE, T_IF, T_ELSE, T_FOR, T_FUNC, T_VAR, T_SIZEOF, T_GOTO, T_LABEL, T_TYPEDEF, T_EXTERN, T_PUBLIC,

    T_STRUCT, T_UNION, T_ENUM
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
