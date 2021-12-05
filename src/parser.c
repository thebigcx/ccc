#include <parser.h>
#include <lexer.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

struct parser
{
    struct token *toks;
    size_t i;
};

static struct parser s_parser;

static struct token *next()
{
    return &s_parser.toks[++s_parser.i];
}

static struct token *curr()
{
    return &s_parser.toks[s_parser.i];
}

// Terminator
static int termin()
{
    return curr()->type == T_SEMI;
}

static int expect(int t)
{
    if (curr()->type != t) return -1;
    next();
    return 0;
}

static int operator()
{
    int op = -1;
    switch (curr()->type)
    {
        case T_PLUS:  op = OP_PLUS; break;
        case T_MINUS: op = OP_MINUS; break;
        case T_STAR:  op = OP_MUL; break;
        case T_SLASH: op = OP_DIV; break;
        default:
            break; // TODO: error
    }

    next();
    return op;
}

static struct ast *primary()
{
    struct ast *ast = malloc(sizeof(struct ast));

    ast->type = A_INTLIT;
    ast->intlit.ival = curr()->v.ival;

    next();
    return ast;
}

static struct ast *binexpr()
{
    struct ast *lhs = primary();

    if (termin()) return lhs;

    int op = operator();
    struct ast *rhs = binexpr();

    struct ast *expr = malloc(sizeof(struct ast));
    expr->binop.lhs = lhs;
    expr->binop.rhs = rhs;
    expr->binop.op  = op;

    return expr;
}

static int typename()
{
    int type = -1;

    switch (curr()->type)
    {
        case T_INT8:    type = TYPE_INT8; break;
        case T_INT16:   type = TYPE_INT16; break;
        case T_INT32:   type = TYPE_INT32; break;
        case T_INT64:   type = TYPE_INT64; break;
        case T_UINT8:   type = TYPE_UINT8; break;
        case T_UINT16:  type = TYPE_UINT16; break;
        case T_UINT32:  type = TYPE_UINT32; break;
        case T_UINT64:  type = TYPE_UINT64; break;
        case T_FLOAT32: type = TYPE_FLOAT32; break;
        case T_FLOAT64: type = TYPE_FLOAT64; break;
    }

    next();
    return type;
}

static struct ast *decl()
{
    int type = typename();

    const char *name = curr()->v.sval;
    next();

    if (curr()->type == T_LPAREN)
    {
        // TODO: parameters
        expect(T_RPAREN);

        struct ast *ast = malloc(sizeof(struct ast));
        ast->type = A_FUNCDEF;
        
        ast->funcdef.rettype = type;
        //ast->funcdef.block   = block_statement();

        return ast;
    }
    else
    {
        struct ast *ast = malloc(sizeof(struct ast));
        ast->type = A_VARDEF;

        ast->vardef.name = strdup(name);
        ast->vardef.type = type; // TODO: assignent

        return ast;
    }
}

int parse(struct token *toks, struct ast *ast)
{
    s_parser.toks = toks;
    s_parser.i    = 0;
    
    struct ast *tree = decl();
    memcpy(ast, tree, sizeof(struct ast));

    return 0;
}
