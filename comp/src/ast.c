#include "ast.h"

#include <stdlib.h>

struct ast *mkast(int type)
{
    struct ast *ast = calloc(1, sizeof(struct ast));
    ast->type = type;
    return ast;
}

struct ast *mkunary(int op, struct ast *val, struct type t)
{
    struct ast *ast = mkast(A_UNARY);
    ast->vtype      = t;
    ast->unary.op   = op;
    ast->unary.val  = val;
    return ast;
}

struct ast *mkintlit(unsigned long val, struct type t)
{
    struct ast *ast  = mkast(A_INTLIT);
    ast->vtype       = t;
    ast->intlit.ival = val;
    return ast;
}

struct ast *mkbinop(int op, struct ast *lhs, struct ast *rhs, struct type t)
{
    struct ast *ast = mkast(A_BINOP);
    ast->vtype      = t;
    ast->binop.op   = op;
    ast->binop.lhs  = lhs;
    ast->binop.rhs  = rhs;
    return ast;
}
