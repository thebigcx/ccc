#pragma once

enum OPERATOR
{
    OP_PLUS,
    OP_MINUS,
    OP_MUL,
    OP_DIV
};

enum AST_TYPE
{
    A_BINOP,
    A_INTLIT,
    A_FUNCDEF,
    A_VARDEF,
    A_BLOCK
};

enum TYPENAME
{
    TYPE_INT8, TYPE_INT16, TYPE_INT32, TYPE_INT64,
    TYPE_UINT8, TYPE_UINT16, TYPE_UINT32, TYPE_UINT64,
    TYPE_FLOAT32, TYPE_FLOAT64
};

struct ast
{
    int type;

    union
    {
        struct
        {
            int op;
            struct ast *lhs, *rhs;
        } binop;

        struct
        {
            unsigned long ival;
        } intlit;
        
        struct
        {
            int rettype; // Return type
            // TODO: parameters
            struct ast *block;
        } funcdef;

        struct // TODO: storage class
        {
            int type;
            char *name;
        } vardef;
    };
};

struct token;

int parse(struct token *toks, struct ast *ast);
