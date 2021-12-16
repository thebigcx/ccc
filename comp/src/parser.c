#include <parser.h>
#include <lexer.h>

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

struct parser
{
    struct token *toks;
    size_t i;
    struct symtable *currscope;
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

// Acts like postfix '++'
static struct token *postnext()
{
    return &s_parser.toks[s_parser.i++];
}

static void error(const char *msg, ...)
{
    printf("\033[1;31merror: \033[37mat line %d: \033[22m", curr()->line);

    va_list list;
    va_start(list, msg);
    vprintf(msg, list);
    va_end(list);

    exit(-1);
}

// Terminator
static int termin()
{
    int t = curr()->type;
    return t == T_SEMI || t == T_RPAREN || t == T_COMMA || t == T_RBRACK;
}

static const char *tokstrs[] =
{
    [T_EOF]     = "EOF",
    [T_PLUS]    = "+",
    [T_MINUS]   = "-",
    [T_STAR]    = "*",
    [T_SLASH]   = "/",
    [T_INTLIT]  = "int literal",
    [T_STRLIT]  = "string literal",
    [T_SEMI]    = ";",
    [T_COMMA]   = ",",
    [T_AMP]     = "&",
    [T_COLON]   = ":",
    [T_IDENT]   = "identifer",
    [T_EQ]      = "=",
    [T_EQEQ]    = "==",
    [T_NEQ]     = "!=",
    [T_GT]      = ">",
    [T_LT]      = "<",
    [T_GTE]     = ">=",
    [T_LTE]     = "<=",
    [T_NOT]     = "!",
    [T_LAND]    = "&&",
    [T_LOR]     = "||",
    [T_BITOR]   = "|",
    [T_BITXOR]  = "^",
    [T_COMP]    = "~",
    [T_TERNARY] = "?",
    [T_INT8]    = "int8",
    [T_INT16]   = "int16",
    [T_INT32]   = "int32",
    [T_INT64]   = "int64",
    [T_UINT8]   = "uint8",
    [T_UINT16]  = "uint16",
    [T_UINT32]  = "uint32",
    [T_UINT64]  = "uint64",
    [T_FLOAT32] = "float32",
    [T_FLOAT64] = "float64",
    [T_LPAREN]  = "(",
    [T_RPAREN]  = ")",
    [T_LBRACK]  = "[",
    [T_RBRACK]  = "]",
    [T_LBRACE]  = "{",
    [T_RBRACE]  = "}",
    [T_ASM]     = "asm",
    [T_RETURN]  = "return",
    [T_WHILE]   = "while",
    [T_IF]      = "if",
    [T_ELSE]    = "else",
    [T_FOR]     = "for",
    [T_FUNC]    = "fn",
    [T_VAR]     = "var",
    [T_SIZEOF]  = "sizeof",
    [T_GOTO]    = "goto",
    [T_LABEL]   = "label"
};

static int expectcurr(int t)
{
    if (curr()->type != t)
    {
        error("Expected '%s', got '%s'\n", tokstrs[t], tokstrs[curr()->type]);
        return -1;
    }
    return 0;
}

static int expect(int t)
{
    expectcurr(t);
    next();
    return 0;
}

static int operator(int tok)
{
    switch (tok)
    {
        case T_PLUS:  return OP_PLUS;
        case T_MINUS: return OP_MINUS;
        case T_STAR:  return OP_MUL;
        case T_SLASH: return OP_DIV;
        case T_EQ:    return OP_ASSIGN;
        case T_EQEQ:  return OP_EQUAL;
        case T_NEQ:   return OP_NEQUAL;
        case T_GT:    return OP_GT;
        case T_LT:    return OP_LT;
        case T_GTE:   return OP_GTE;
        case T_LTE:   return OP_LTE;
        case T_LAND:  return OP_LAND;
        case T_LOR:   return OP_LOR;
    }
    return -1;
}

static int istype(int token)
{
    return token == T_INT8  || token == T_INT16  || token == T_INT32  || token == T_INT64  || 
           token == T_UINT8 || token == T_UINT16 || token == T_UINT32 || token == T_UINT64 || 
           token == T_FLOAT32 || token == T_FLOAT64;
}

static struct type parsetype()
{
    struct type t = { 0 };

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
        case T_STAR:    t.name = TYPE_VOID; break;
        default:
            error("Expected typename, got '%s'\n", tokstrs[curr()->type]);
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

// Check implicit conversion
static int check_impconv(struct type t1, struct type t2)
{
    // TODO: temporary
    return t1.name != TYPE_STRUCT && t2.name != TYPE_STRUCT;
}

static int isintegral(struct type t)
{
    return t.ptr || (t.name != TYPE_STRUCT && t.name != TYPE_UNION);
}

static struct ast *binexpr();
static struct ast *primary();

static struct ast *parenexpr()
{
    next();

    struct ast *ast;
    if (istype(curr()->type))
    {
        struct type t = parsetype();
        if (!isintegral(t) || t.arrlen)
            error("Cannot cast to non-integral type\n");

        expect(T_RPAREN);
        struct ast *val = primary();

        ast = calloc(1, sizeof(struct ast));
        ast->type      = A_CAST;
        ast->cast.type = t;
        ast->cast.val  = val;
        return ast;
    }
    else
    {
        ast = binexpr(0);
    }

    return ast;
}

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
    else if (curr()->type == T_NOT)
    {
        next();
        ast->type      = A_UNARY;
        ast->unary.op  = OP_NOT;
        ast->unary.val = primary();
        return ast;
    }
    else if (curr()->type == T_MINUS)
    {
        next();
        ast->type      = A_UNARY;
        ast->unary.op  = OP_MINUS;
        ast->unary.val = primary();
        return ast;
    }
    else if (curr()->type == T_SIZEOF)
    {
        next();
        ast->type       = A_SIZEOF;
        ast->sizeofop.t = parsetype();
        return ast;
    }
    else if (curr()->type == T_INTLIT)
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

        if (!sym_lookup(s_parser.currscope, name))
            error("Use of undeclared symbol '%s'\n", name);

        next();
        if (curr()->type == T_LPAREN)
        {
            struct sym *sym = sym_lookup(s_parser.currscope, name);

            next();
            unsigned int i;
            for (i = 0; curr()->type != T_RPAREN; i++)
            {
                ast->call.params = realloc(ast->call.params, (ast->call.paramcnt + 1) * sizeof(struct ast*));
                ast->call.params[ast->call.paramcnt++] = binexpr(0);

                if (curr()->type != T_RPAREN) expect(T_COMMA);
            }
            expect(T_RPAREN);

            if (i < sym->func.paramcnt)
                error("Too few parameters in call to function '%s'\n", name);
            else if (i > sym->func.paramcnt)
                error("Too many parameters in call to function '%s'\n", name);

            ast->type = A_CALL;
            ast->call.name = strdup(name);
        }
        else
        {
            ast->type = A_IDENT;
            ast->ident.name = strdup(name);
        }
    }
    else if (curr()->type == T_LPAREN)
    {
        free(ast);
        ast = parenexpr();
    }
    else
    {
        error("Expected primary expression, got '%s'\n", tokstrs[curr()->type]);
    }

    /*if (curr()->type == T_LBRACK)
    {
        // TODO: this could be better written as an array access's explicit form:
        // arr[5] is equivalent to *(&arr + 5)
        next();

        struct ast *arr = calloc(1, sizeof(struct ast));
        arr->type       = A_ARRACC;
        arr->arracc.arr = ast;
        arr->arracc.off = binexpr(0);

        expect(T_RBRACK);
        return arr;
    }*/

    return ast;
}

static int rightassoc(int op)
{
    return op == OP_ASSIGN;
}

static int opprec[] =
{
    [OP_ASSIGN] = 1,
    [OP_LOR]    = 2,
    [OP_LAND]   = 3,
    [OP_PLUS]   = 4,
    [OP_MINUS]  = 4,
    [OP_MUL]    = 5,
    [OP_DIV]    = 5,
    [OP_EQUAL]  = 6,
    [OP_NEQUAL] = 6,
    [OP_LT]     = 7,
    [OP_GT]     = 7,
    [OP_LTE]    = 7,
    [OP_GTE]    = 7,
};

static struct ast *binexpr(int ptp)
{
    struct ast *rhs;
    struct ast *lhs = primary();

    if (termin()) return lhs;

    int op;
    if ((op = operator(curr()->type)) == -1)
    {
        error("Expected operator, got '%s'\n", tokstrs[curr()->type]);
    }

    while (opprec[op] > ptp || (rightassoc(op) && opprec[op] == ptp))
    {
        next();
        rhs = binexpr(opprec[op]);

        struct ast *expr = calloc(1, sizeof(struct ast));
        expr->type = A_BINOP;
        expr->binop.lhs = lhs;
        expr->binop.rhs = rhs;
        expr->binop.op  = op;

        lhs->lvalue = 1;
        rhs->lvalue = 0;

        lhs = expr;
        if (termin()) return lhs;

        if ((op = operator(curr()->type)) == -1)
        {
            error("Expected operator, got '%s'\n", tokstrs[curr()->type]);
        }
    }

    return lhs;
}

static struct ast *inlineasm()
{
    struct ast *ast = calloc(1, sizeof(struct ast));
    ast->type = A_ASM;
    
    ast->inasm.code = strdup(curr()->v.sval);

    next();
    return ast;
}

static struct ast *block();
static struct ast *vardecl();

static struct ast *funcdecl()
{
    struct sym sym = { 0 };
    sym.attr = SYM_FUNC;

    struct ast *ast = calloc(1, sizeof(struct ast));
    ast->type = A_FUNCDEF;

    ast->funcdef.name = strdup(curr()->v.sval);
    sym.name = strdup(ast->funcdef.name);
    
    expect(T_IDENT);
    expect(T_LPAREN);

    while (curr()->type != T_RPAREN)
    {
        next();
        expect(T_COLON);
        sym.func.params[sym.func.paramcnt++] = parsetype();
        if (curr()->type != T_RPAREN) expect(T_COMMA);
    }
    
    expect(T_RPAREN);

    if (curr()->type == T_COLON)
    {
        next();
        sym.func.ret = parsetype();
    }
    else sym.func.ret = (struct type) { .name = TYPE_VOID };

    sym_putglob(s_parser.currscope, &sym);

    if (curr()->type == T_LBRACE)
    {
        next();
        ast->funcdef.block = block(SYMTAB_FUNC);
        expect(T_RBRACE);
    }
    else
    {
        expect(T_SEMI);
        free(ast);
        return NULL;
    }

    return ast;
}

static struct ast *vardecl()
{
    const char *vars[128] = { NULL };
    unsigned int varcnt = 0;

    do
    {
        vars[varcnt++] = curr()->v.sval;
        expect(T_IDENT);
    } while (postnext()->type == T_COMMA);
    
    struct type type = parsetype();

    for (unsigned int i = 0; i < varcnt; i++)
        sym_put(s_parser.currscope, vars[i], type, 0);

    struct ast *ast = calloc(1, sizeof(struct ast));
    ast->type = A_VARDEF;
    return ast;
}

static struct ast *return_statement()
{
    next();

    struct ast *ast = calloc(1, sizeof(struct ast));
    ast->type = A_RETURN;
    
    if (curr()->type != T_SEMI)
        ast->ret.val = binexpr(0);

    return ast;
}

static struct ast *if_statement()
{
    next();
    expect(T_LPAREN);

    struct ast *ast = calloc(1, sizeof(struct ast));
    ast->type = A_IFELSE;

    ast->ifelse.cond = binexpr(0);
    expect(T_RPAREN);

    expect(T_LBRACE);
    ast->ifelse.ifblock = block(SYMTAB_BLOCK);
    expect(T_RBRACE);

    if (curr()->type == T_ELSE)
    {
        next();
        expect(T_LBRACE);
        ast->ifelse.elseblock = block(SYMTAB_BLOCK);
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

    ast->whileloop.cond = binexpr(0);
    expect(T_RPAREN);

    expect(T_LBRACE);
    ast->whileloop.body = block(SYMTAB_BLOCK);
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

    ast->forloop.cond = binexpr(0);
    expect(T_SEMI);

    ast->forloop.update = statement();
    expect(T_RPAREN);

    expect(T_LBRACE);
    ast->forloop.body = block(SYMTAB_BLOCK);
    expect(T_RBRACE);

    return ast;
}

static struct ast *label()
{
    expect(T_LABEL);
    struct ast *ast = calloc(1, sizeof(struct ast));
    ast->type = A_LABEL;
    ast->label.name = strdup(curr()->v.sval);
    
    next();
    expect(T_COLON);
    return ast;
}

static struct ast *gotolbl()
{
    expect(T_GOTO);
    struct ast *ast = calloc(1, sizeof(struct ast));
    ast->type = A_GOTO;
    ast->gotolbl.label = strdup(curr()->v.sval);

    next();
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
    else if (curr()->type == T_LABEL)
        ast = label();
    else if (curr()->type == T_GOTO)
        ast = gotolbl();
    else
        ast = binexpr(0);
    
    return ast;
}

static struct ast *block(int type)
{
    struct ast *block = calloc(1, sizeof(struct ast));
    block->type = A_BLOCK;

    block->block.symtab.type = type;

    block->block.symtab.parent = s_parser.currscope;
    s_parser.currscope = &block->block.symtab;

    while (curr()->type != T_RBRACE && curr()->type != T_EOF)
    {
        struct ast *ast = statement();
        if (!ast) continue; // Could be a declaration - TODO: distinguish global block from function blocks

        if (ast->type != A_FUNCDEF && ast->type != A_ASM && ast->type != A_IFELSE
            && ast->type != A_FOR && ast->type != A_WHILE && ast->type != A_LABEL)
            expect(T_SEMI);

        block->block.statements = realloc(block->block.statements, ++block->block.cnt * sizeof(struct ast*));
        block->block.statements[block->block.cnt - 1] = ast;
    }
    
    s_parser.currscope = s_parser.currscope->parent;
    return block;
}

int parse(struct token *toks, struct ast *ast)
{
    s_parser.toks      = toks;
    s_parser.i         = 0;
    s_parser.currscope = NULL;
    
    struct ast *tree = block(SYMTAB_GLOB);
    memcpy(ast, tree, sizeof(struct ast));

    return 0;
}
