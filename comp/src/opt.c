#include <opt.h>
#include <parser.h>

struct ast *mkintlit(unsigned int val, struct type t)
{
    struct ast *ast  = mkast(A_INTLIT);
    ast->vtype       = t;
    ast->intlit.ival = val;
    return ast;
}

struct ast *fold(struct ast *ast)
{
    if (ast->type == A_UNARY)
    {
        struct ast *val = ast->unary.val = fold(ast->unary.val);
        if (val->type != A_INTLIT) return ast;

        switch (ast->unary.op)
        {
            case OP_BITNOT: return mkintlit(~val->intlit.ival, val->vtype);
            case OP_LOGNOT: return mkintlit(!val->intlit.ival, val->vtype);
        }
    }
    else if (ast->type == A_BINOP)
    {
        struct ast *lhs = ast->binop.lhs = fold(ast->binop.lhs);
        struct ast *rhs = ast->binop.rhs = fold(ast->binop.rhs);

        // Prune branches if possible
        if (lhs->type == A_INTLIT && lhs->intlit.ival == 0)
        {
            switch (ast->binop.op)
            {
                case OP_PLUS: return rhs; // x + 0 = x
                case OP_MUL:
                case OP_MOD:
                case OP_DIV:
                case OP_SHL:
                case OP_SHR:
                case OP_BITAND: return mkintlit(0, lhs->vtype); // 0 * x, 0 << x, 0 & x, always 0
            }
        }
        else if (rhs->type == A_INTLIT && rhs->intlit.ival == 0)
        {
            switch (ast->binop.op)
            {
                case OP_PLUS:
                case OP_MINUS:
                case OP_SHL:
                case OP_SHR:
                case OP_BITOR:
                case OP_BITXOR: return lhs; // x + 0, x << 0, x | 0, just return x
                case OP_MUL:
                case OP_BITAND: return mkintlit(0, lhs->vtype); // x * 0, x & 0 is always 0
            }
        }

        if (lhs->type != A_INTLIT || rhs->type != A_INTLIT) return ast;

        switch (ast->binop.op)
        {
            case OP_PLUS:   return mkintlit(lhs->intlit.ival +  rhs->intlit.ival, lhs->vtype);
            case OP_MINUS:  return mkintlit(lhs->intlit.ival -  rhs->intlit.ival, lhs->vtype);
            case OP_MUL:    return mkintlit(lhs->intlit.ival *  rhs->intlit.ival, lhs->vtype);
            case OP_SHL:    return mkintlit(lhs->intlit.ival << rhs->intlit.ival, lhs->vtype);
            case OP_SHR:    return mkintlit(lhs->intlit.ival >> rhs->intlit.ival, lhs->vtype);
            case OP_BITAND: return mkintlit(lhs->intlit.ival &  rhs->intlit.ival, lhs->vtype);
            case OP_BITOR:  return mkintlit(lhs->intlit.ival |  rhs->intlit.ival, lhs->vtype);
            case OP_BITXOR: return mkintlit(lhs->intlit.ival ^  rhs->intlit.ival, lhs->vtype);
            
            case OP_DIV:
                if (rhs->intlit.ival) return mkintlit(lhs->intlit.ival / rhs->intlit.ival, lhs->vtype);
            case OP_MOD:
                if (rhs->intlit.ival) return mkintlit(lhs->intlit.ival % rhs->intlit.ival, lhs->vtype);
        }
    }
    else if (ast->type == A_SCALE)
    {
        struct ast *val = fold(ast->scale.val);
        if (val->type != A_INTLIT) return ast;

        struct ast *res = mkast(A_INTLIT);
        res->vtype = ast->vtype;
        res->intlit.ival = ast->scale.num * val->intlit.ival;
        return res;
    }

    return ast;
}