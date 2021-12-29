#pragma once

#include <type.h>
#include <sym.h>

enum OPERATOR
{
    OP_LOGNOT,
    OP_BITNOT,
    OP_DEREF,
    OP_ADDROF,
    OP_MUL,
    OP_DIV,
    OP_MOD,
    OP_PLUS,
    OP_MINUS,
    OP_SHL,
    OP_SHR,
    OP_LT,
    OP_LTE,
    OP_GT,
    OP_GTE,
    OP_EQUAL,
    OP_NEQUAL,
    OP_BITAND,
    OP_BITXOR,
    OP_BITOR,
    OP_LAND,
    OP_LOR,
    OP_TERNARY,
    OP_ASSIGN,
    OP_PLUSEQ,
    OP_MINUSEQ,
    OP_MULEQ,
    OP_DIVEQ,
    OP_MODEQ,
    OP_SHLEQ,
    OP_SHREQ,
    OP_BITANDEQ,
    OP_BITXOREQ,
    OP_BITOREQ,
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
    A_SIZEOF,
    A_LABEL,
    A_GOTO,
    A_CAST,
    A_PREINC,
    A_POSTINC,
    A_PREDEC,
    A_POSTDEC,
    A_SCALE,
    A_TERNARY
};

struct rostr
{
    char *val;
    int  lbl;
};

struct ast
{
    int type, lvalue; // TODO: I don't like this 'lvalue' nonsense for determining if a dereference is a load or store - come up with a better way

    struct type vtype;

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
            char *name;
            struct ast *block;
            int endlbl;
            char *params[6];
        } funcdef;

        struct
        {
            char *name;
        } ident;

        struct
        {
            struct ast **statements;
            unsigned int cnt;
            struct symtable symtab;
            struct rostr *strs;
            unsigned int strcnt;
        } block;

        struct
        {
            char *code;
        } inasm;

        struct
        {
            struct ast *ast;
            struct ast **params;
            unsigned int paramcnt;
        } call;

        struct
        {
            struct ast *val;
            struct ast *func;
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
            unsigned int idx;
        } strlit;

        struct
        {
            struct type t;
        } sizeofop;

        struct
        {
            char *name;
        } label;

        struct
        {
            char *label;
        } gotolbl;

        struct
        {
            struct type type;
            struct ast *val;
        } cast;

        struct
        {
            struct ast *val;
        } incdec; // ++, --

        struct
        {
            struct ast *val;
            unsigned int num;
        } scale;

        struct
        {
            struct ast *cond, *lhs, *rhs;
        } ternary;
    };
};

struct ast *mkast(int type);
struct ast *mkunary(int op, struct ast *val);