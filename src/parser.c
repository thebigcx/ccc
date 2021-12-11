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
    int t = curr()->type;
    return t == T_SEMI || t == T_RPAREN || t == T_COMMA;
}

static int expect(int t)
{
    if (curr()->type != t)
    {
        printf("Expected '%d', got '%d'\n", t, curr()->type);
        abort();
        return -1;
    }

    next();
    return 0;
}

static int operator()
{
    int op = -1;
    switch (curr()->type)
    {
        case T_PLUS:  op = OP_PLUS;   break;
        case T_MINUS: op = OP_MINUS;  break;
        case T_STAR:  op = OP_MUL;    break;
        case T_SLASH: op = OP_DIV;    break;
        case T_EQ:    op = OP_ASSIGN; break;
        case T_EQEQ:  op = OP_EQUAL;  break;
        case T_NEQ:   op = OP_NEQUAL; break;
        case T_GT:    op = OP_GT;     break;
        case T_LT:    op = OP_LT;     break;
        case T_GTE:   op = OP_GTE;    break;
        case T_LTE:   op = OP_LTE;    break;
        default:
            break; // TODO: error
    }

    next();
    return op;
}

static struct ast *binexpr();

static struct ast *primary()
{
    struct ast *ast = calloc(1, sizeof(struct ast));

    if (curr()->type == T_AMP)
    {
        next();
        ast->type      = A_UNARY;
        ast->unary.op  = OP_ADDROF;
        ast->unary.val = primary();
        return ast;
    }
    else if (curr()->type == T_STAR)
    {
        next();
        ast->type      = A_UNARY;
        ast->unary.op  = OP_DEREF;
        ast->unary.val = primary();
        return ast;
    }

    if (curr()->type == T_INTLIT)
    {
        ast->type = A_INTLIT;
        ast->intlit.ival = curr()->v.ival;
        next();
    }
    else if (curr()->type == T_STRLIT)
    {
        ast->type = A_STRLIT;
        ast->strlit.str = strdup(curr()->v.sval);
        next();
    }
    else if (curr()->type == T_IDENT)
    {
        char *name = curr()->v.sval;

        next();
        if (curr()->type == T_LPAREN)
        {
            next();
            while (curr()->type != T_RPAREN)
            {
                ast->call.params = realloc(ast->call.params, (ast->call.paramcnt + 1) * sizeof(struct ast*));
                ast->call.params[ast->call.paramcnt++] = binexpr();
            }
            expect(T_RPAREN);

            ast->type = A_CALL;
            ast->call.name = strdup(name);
        }
        else
        {
            ast->type = A_IDENT;
            ast->ident.name = strdup(name);
        }
    }

    return ast;
}

static struct ast *binexpr()
{
    struct ast *lhs = primary();

    if (termin()) return lhs;

    int op = operator();
    struct ast *rhs = binexpr();

    struct ast *expr = malloc(sizeof(struct ast));
    expr->type = A_BINOP;

    expr->binop.lhs = lhs;
    expr->binop.rhs = rhs;
    expr->binop.op  = op;

    return expr;
}

static struct ast *inlineasm()
{
    struct ast *ast = malloc(sizeof(struct ast));
    ast->type = A_ASM;
    
    ast->inasm.code = strdup(curr()->v.sval);

    next();
    return ast;
}

static struct type parsetype()
{
    struct type t;

    switch (curr()->type)
    {
        case T_INT8:    t.name = TYPE_INT8; break;
        case T_INT16:   t.name = TYPE_INT16; break;
        case T_INT32:   t.name = TYPE_INT32; break;
        case T_INT64:   t.name = TYPE_INT64; break;
        case T_UINT8:   t.name = TYPE_UINT8; break;
        case T_UINT16:  t.name = TYPE_UINT16; break;
        case T_UINT32:  t.name = TYPE_UINT32; break;
        case T_UINT64:  t.name = TYPE_UINT64; break;
        case T_FLOAT32: t.name = TYPE_FLOAT32; break;
        case T_FLOAT64: t.name = TYPE_FLOAT64; break;
    }

    while (next()->type == T_STAR) t.ptr++;

    if (curr()->type == T_LBRACK)
    {
        next();
        t.arrlen = curr()->v.ival;
        expect(T_INTLIT);
        expect(T_RBRACK);
    }

    return t;
}

static int istype(int token)
{
    return token == T_INT8  || token == T_INT16  || token == T_INT32  || token == T_INT64  || 
           token == T_UINT8 || token == T_UINT16 || token == T_UINT32 || token == T_UINT64 || 
           token == T_FLOAT32 || token == T_FLOAT64;
}

static struct ast *block();
static struct ast *vardecl();

static struct ast *funcdecl()
{
    struct ast *ast = calloc(1, sizeof(struct ast));
    ast->type = A_FUNCDEF;

    ast->funcdef.name = strdup(curr()->v.sval);
    
    next();
    expect(T_LPAREN);

    while (curr()->type != T_RPAREN)
    {
        ast->funcdef.params[ast->funcdef.paramcnt++] = vardecl();
        if (curr()->type != T_RPAREN) expect(T_COMMA);
    }
    
    expect(T_RPAREN);

    if (curr()->type == T_COLON)
    {
        next();
        ast->funcdef.rettype = parsetype();
    }
    else ast->funcdef.rettype = (struct type) { .name = TYPE_VOID };

    expect(T_LBRACE);
    ast->funcdef.block   = block();
    expect(T_RBRACE);

    return ast;
}

static struct ast *vardecl()
{
    const char *name = curr()->v.sval;

    struct type type = (struct type) { .name = TYPE_VOID };
    if (next()->type == T_COLON)
    {
        next();
        type = parsetype();
    }
    
    struct ast *ast = malloc(sizeof(struct ast));
    ast->type = A_VARDEF;

    ast->vardef.name = strdup(name);
    ast->vardef.type = type; // TODO: assignent

    return ast;
}

static struct ast *return_statement()
{
    next();

    struct ast *ast = calloc(1, sizeof(struct ast));
    ast->type = A_RETURN;
    
    if (curr()->type != T_SEMI)
        ast->ret.val = binexpr();

    return ast;
}

static struct ast *if_statement()
{
    next();
    expect(T_LPAREN);

    struct ast *ast = calloc(1, sizeof(struct ast));
    ast->type = A_IFELSE;

    ast->ifelse.cond = binexpr();
    expect(T_RPAREN);

    expect(T_LBRACE);
    ast->ifelse.ifblock = block();
    expect(T_RBRACE);

    if (curr()->type == T_ELSE)
    {
        next();
        expect(T_LBRACE);
        ast->ifelse.elseblock = block();
        expect(T_RBRACE);
    }

    return ast;
}

static struct ast *while_statement()
{
    next();
    expect(T_LPAREN);

    struct ast *ast = calloc(1, sizeof(struct ast));
    ast->type = A_WHILE;

    ast->whileloop.cond = binexpr();
    expect(T_RPAREN);

    expect(T_LBRACE);
    ast->whileloop.body = block();
    expect(T_RBRACE);

    return ast;
}

static struct ast *statement();

static struct ast *for_statement()
{
    next();
    expect(T_LPAREN);

    struct ast *ast = calloc(1, sizeof(struct ast));
    ast->type = A_FOR;

    ast->forloop.init = statement();
    expect(T_SEMI);

    ast->forloop.cond = binexpr();
    expect(T_SEMI);

    ast->forloop.update = statement();
    expect(T_RPAREN);

    expect(T_LBRACE);
    ast->forloop.body = block();
    expect(T_RBRACE);

    return ast;
}

static struct ast *statement()
{
    struct ast *ast;

    if (curr()->type == T_ASM)
        ast = inlineasm();
    else if (curr()->type == T_FUNC)
    {
        next();
        ast = funcdecl();
    }
    else if (curr()->type == T_VAR)
    {
        next();
        ast = vardecl();
    }
    else if (curr()->type == T_RETURN)
        ast = return_statement();
    else if (curr()->type == T_IF)
        ast = if_statement();
    else if (curr()->type == T_WHILE)
        ast = while_statement();
    else if (curr()->type == T_FOR)
        ast = for_statement();
    else
        ast = binexpr();
    
    return ast;
}

static struct ast *block()
{
    struct ast *block = calloc(1, sizeof(struct ast));
    block->type = A_BLOCK;

    while (curr()->type != T_RBRACE && curr()->type != T_EOF)
    {
        struct ast *ast = statement();

        if (ast->type == A_VARDEF || ast->type == A_BINOP || ast->type == A_RETURN || ast->type == A_CALL)
            expect(T_SEMI);

        block->block.statements = realloc(block->block.statements, ++block->block.cnt * sizeof(struct ast*));
        block->block.statements[block->block.cnt - 1] = ast;
    }

    return block;
}

int parse(struct token *toks, struct ast *ast)
{
    s_parser.toks = toks;
    s_parser.i    = 0;
    
    struct ast *tree = block();
    memcpy(ast, tree, sizeof(struct ast));

    return 0;
}
