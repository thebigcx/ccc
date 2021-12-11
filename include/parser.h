#pragma once

enum OPERATOR
{
    OP_PLUS,
    OP_MINUS,
    OP_MUL,
    OP_DIV,
    OP_ASSIGN,
    OP_EQUAL,
    OP_NEQUAL,
    OP_GT,
    OP_LT,
    OP_GTE,
    OP_LTE,
    OP_ADDROF,
    OP_DEREF,
};

enum AST_TYPE
{
    A_BINOP,
    A_INTLIT,
    A_FUNCDEF,
    A_VARDEF,
    A_BLOCK,
    A_IDENT,
    A_ASM,
    A_CALL,
    A_RETURN,
    A_IFELSE,
    A_WHILE,
    A_FOR,
    A_UNARY,
    A_STRLIT,
    A_SIZEOF
};

enum TYPENAME
{
    TYPE_INT8, TYPE_INT16, TYPE_INT32, TYPE_INT64,
    TYPE_UINT8, TYPE_UINT16, TYPE_UINT32, TYPE_UINT64,
    TYPE_FLOAT32, TYPE_FLOAT64, TYPE_VOID
};

struct type
{
    int ptr, name, arrlen;
};

struct ast
{
    int type, lvalue; // TODO: I don't like this 'lvalue' nonsense for determining if a dereference is a load or store - come up with a better way

    union
    {
        struct
        {
            int op;
            struct ast *lhs, *rhs;
        } binop;

        struct
        {
            int op;
            struct ast *val;
        } unary;

        struct
        {
            unsigned long ival;
        } intlit;
        
        struct
        {
            struct type rettype;
            struct ast *params[6];
            unsigned int paramcnt;
            struct ast *block;
            char *name;
        } funcdef;

        struct // TODO: storage class
        {
            struct type type;
            char *name;
        } vardef;

        struct
        {
            char *name;
        } ident;

        struct
        {
            struct ast **statements;
            unsigned int cnt;
        } block;

        struct
        {
            char *code;
        } inasm;

        struct
        {
            char *name;
            struct ast **params;
            unsigned int paramcnt;
        } call;

        struct
        {
            struct ast *val;
        } ret;

        struct
        {
            struct ast *cond, *ifblock, *elseblock;
        } ifelse;

        struct
        {
            struct ast *cond, *body;
        } whileloop;

        struct
        {
            struct ast *init, *cond, *update, *body;
        } forloop;

        struct
        {
            char *str;
        } strlit;

        struct
        {
            struct type t;
        } sizeofop;
    };
};

struct token;

int parse(struct token *toks, struct ast *ast);
