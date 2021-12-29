#include <ast.h>
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