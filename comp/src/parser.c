#include <parser.h>
#include <lexer.h>
#include <asm.h>
#include <opt.h>
#include <ast.h>

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
    struct ast      *currfunc;
    struct ast      *globlscope;
};

static struct parser s_parser;

static struct sym *s_typedefs = NULL;
static unsigned int s_typedefcnt = 0;

static struct token *next()
{
    return &s_parser.toks[++s_parser.i];
}

static struct token *back()
{
    return &s_parser.toks[--s_parser.i];
}

static struct token *curr()
{
    return &s_parser.toks[s_parser.i];
}

// Acts like post increment '++'
static struct token *postnext()
{
    return &s_parser.toks[s_parser.i++];
}

static void error(const char *msg, ...)
{
    printf("\033[1;31merror: \033[37mfile '%s' at line %d: \033[22m", curr()->file, curr()->line);

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
    return t == T_SEMI || t == T_RPAREN || t == T_COMMA || t == T_RBRACK || t == T_COLON;
}

static const char *tokstrs[] =
{
    [T_EOF]     = "EOF",
    [T_END]     = "EOF",
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
    [T_DOT]     = ".",
    [T_ARROW]   = "->",
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
    [T_BITNOT]  = "~",
    [T_TERNARY] = "?",
    [T_INC]     = "++",
    [T_DEC]     = "--",
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
        case T_PLUS:    return OP_PLUS;
        case T_MINUS:   return OP_MINUS;
        case T_STAR:    return OP_MUL;
        case T_SLASH:   return OP_DIV;
        case T_MOD:     return OP_MOD;
        case T_PLUSEQ:  return OP_PLUSEQ;
        case T_MINUSEQ: return OP_MINUSEQ;
        case T_MULEQ:   return OP_MULEQ;
        case T_DIVEQ:   return OP_DIVEQ;
        case T_ANDEQ:   return OP_BITANDEQ;
        case T_OREQ:    return OP_BITOREQ;
        case T_XOREQ:   return OP_BITXOREQ;
        case T_SHLEQ:   return OP_SHLEQ;
        case T_SHREQ:   return OP_SHREQ;
        case T_MODEQ:   return OP_MODEQ;
        case T_EQ:      return OP_ASSIGN;
        case T_EQEQ:    return OP_EQUAL;
        case T_NEQ:     return OP_NEQUAL;
        case T_GT:      return OP_GT;
        case T_LT:      return OP_LT;
        case T_GTE:     return OP_GTE;
        case T_LTE:     return OP_LTE;
        case T_LAND:    return OP_LAND;
        case T_LOR:     return OP_LOR;
        case T_TERNARY: return OP_TERNARY;
        case T_SHL:     return OP_SHL;
        case T_SHR:     return OP_SHR;
        case T_AMP:     return OP_BITAND;
        case T_BITOR:   return OP_BITOR;
        case T_BITXOR:  return OP_BITXOR;
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
        case T_INT8:    t.name = TYPE_INT8; next(); break;
        case T_INT16:   t.name = TYPE_INT16; next(); break;
        case T_INT32:   t.name = TYPE_INT32; next(); break;
        case T_INT64:   t.name = TYPE_INT64; next(); break;
        case T_UINT8:   t.name = TYPE_UINT8; next(); break;
        case T_UINT16:  t.name = TYPE_UINT16; next(); break;
        case T_UINT32:  t.name = TYPE_UINT32; next(); break;
        case T_UINT64:  t.name = TYPE_UINT64; next(); break;
        case T_FLOAT32: t.name = TYPE_FLOAT32; next(); break;
        case T_FLOAT64: t.name = TYPE_FLOAT64; next(); break;
        case T_STAR:    t.name = TYPE_VOID; break;
        case T_FUNC: // Parse function signature
        {
            t.name = TYPE_FUNC;
            t.func.ret = calloc(1, sizeof(struct type));
            
            while (next()->type == T_STAR) t.ptr++;
            expect(T_LPAREN);
            
            while (curr()->type != T_RPAREN)
            {
                t.func.params[t.func.paramcnt] = calloc(1, sizeof(struct type));
                *t.func.params[t.func.paramcnt++] = parsetype();
                if (curr()->type != T_RPAREN) expect(T_COMMA);
            }

            next();
            if (curr()->type == T_ARROW)
            {
                expect(T_ARROW);

                *t.func.ret = parsetype();
            }
            goto array;
        }
        case T_IDENT:
        {
            // TODO: need to account for pointer and stuff. Should use a
            // base type with modifications to it
            for (unsigned int i = 0; i < s_typedefcnt; i++)
            {
                if (!strcmp(s_typedefs[i].name, curr()->v.sval))
                {
                    next();
                    t = s_typedefs[i].type;
                    goto done_typename;
                }
            }
            goto error;
        }

        default:
        error:
            error("Expected type, got '%s'\n", tokstrs[curr()->type]);
    }

done_typename:
    while (curr()->type == T_STAR)
    {
        t.ptr++;
        next();
    }

array:
    if (curr()->type == T_LBRACK)
    {
        next();
        t.arrlen = curr()->v.ival;
        expect(T_INTLIT);
        expect(T_RBRACK);
    }

    return t;
}

static int isintegral(struct type t)
{
    return t.name != TYPE_STRUCT && t.name != TYPE_UNION && t.name != TYPE_FUNC && !t.ptr && !t.arrlen;
}

static struct type mktype(int name, int arrlen, int ptr)
{
    return (struct type) { .name = name, .arrlen = arrlen, .ptr = ptr };
}

static int type_compatible(struct type t1, struct type t2)
{
    if (t1.ptr && t2.ptr) return 1;
    if (isintegral(t1) && isintegral(t2)) return 1;

    return 0;
}

static void add_typedef(const char *name, struct type type)
{
    s_typedefs = realloc(s_typedefs, (s_typedefcnt + 1) * sizeof(struct sym));
    s_typedefs[s_typedefcnt++] = (struct sym)
    {
        .name = strdup(name),
        .type = type
    };
}

static struct ast *binexpr();
static struct ast *primary();
static struct ast *pre();
static struct ast *post(struct ast *ast);

static struct ast *parenexpr()
{
    next();
    if (istype(curr()->type))
    {
        struct type t = parsetype();
        if (!isintegral(t) && !t.ptr)
            error("Cannot cast to non-integral or pointer type\n");

        expect(T_RPAREN);
        struct ast *val = pre();

        struct ast *ast = mkast(A_CAST);
        ast->vtype = t;
        ast->cast.type = t;
        ast->cast.val  = val;
        return ast;
    }
    else
    {
        struct ast *ast = binexpr();
        expect(T_RPAREN);
        return ast;
    }
}

static struct ast *pre()
{
    struct ast *ast, *val;

    switch (curr()->type)
    {
        case T_AMP: // TODO: type checking: must be an lvalue
        {
            next();
            val = pre();
            if (val->type == A_UNARY && val->unary.op == OP_DEREF) return val->unary.val;
            
            ast = mkunary(OP_ADDROF, val, val->vtype);
            if (val->type != A_IDENT)
                error("Invalid use of address-of operator.\n");
            ast->vtype.ptr++;
            return ast;
        }

        case T_STAR:
            next();
            val = pre();
            ast = mkunary(OP_DEREF, val, val->vtype);
            if (!val->vtype.ptr)
                error("Cannot dereference non-pointer type.\n");
            ast->vtype.ptr--;
            return ast;
        case T_NOT:
            next();
            val = pre();
            ast = mkunary(OP_LOGNOT, val, val->vtype);
            if (!isintegral(ast->vtype) && !ast->vtype.ptr)
                error("Logical not on non-integral or pointer type.\n");
            return ast;
        case T_BITNOT:
            next();
            val = pre();
            ast = mkunary(OP_BITNOT, val, val->vtype);
            if (!isintegral(ast->vtype))
                error("Bitwise not on non-integral type.\n");
            return ast;
        case T_MINUS:
            next();
            val = pre();
            ast = mkunary(OP_MINUS, val, val->vtype);
            ast->vtype = ast->unary.val->vtype;
            if (!isintegral(ast->vtype))
                error("Unary minus on non-integral type.\n");
            return ast;
        case T_INC:
        case T_DEC:
            ast = mkast(curr()->type == T_INC ? A_PREINC : A_PREDEC);
            next();
            ast->incdec.val = pre();
            ast->vtype = ast->incdec.val->vtype;
            return ast;
        default:
            return post(primary());
    }
}

static struct ast *memaccess(struct ast *ast)
{
    int ptr = curr()->type == T_ARROW;

    struct ast *address;
    if (ptr)
        address = ast;
    else if (ast->type == A_UNARY && ast->unary.op == OP_DEREF)
        address = ast->unary.val;
    else
        address = mkunary(OP_ADDROF, ast, mktype(TYPE_UINT64, 0, 0));

    struct type structype = ast->vtype;
    ast = address;

    struct structmem *member;
    while (curr()->type == T_DOT || curr()->type == T_ARROW)
    {
        if (structype.name != TYPE_STRUCT && structype.name != TYPE_UNION)
            error("Member access of non-struct type.\n");

        ptr = curr()->type == T_ARROW;

        if (ptr && !structype.ptr)
            error("Use of arrow operator '->' on non-pointer to struct. Use '.' instead\n");
        else if (!ptr && structype.ptr)
            error("Use of dot operator '.' on pointer to struct. Use '->' instead.\n");

        next();
        const char *name = curr()->v.sval;
        expect(T_IDENT);

        member = NULL;
        for (unsigned i = 0; i < structype.struc.memcnt; i++)
        {
            if (!strcmp(name, structype.struc.members[i].name))
            {
                member = &structype.struc.members[i];
                break;
            }
        }

        if (!member)
            error("Struct does not contain member '%s'\n", name);

        struct ast *off = mkintlit(member->offset, ast->vtype);
        struct ast *add = mkbinop(OP_PLUS, ast, off, off->vtype);

        structype = member->type;
        ast = add;
    }

    struct ast *deref = mkunary(OP_DEREF, ast, member->type);
    if (curr()->type == T_DOT)
        return memaccess(deref);

    return deref;
}

//  x[10].x = 10;
//  *(&(*(&x + 10)) + offsetof(x))) = 10;
//  *(&x + 10 + offsetof(x)) = 10;

static struct ast *post(struct ast *branch)
{
    struct ast *ast = branch;

    while (1)
    {
        switch (curr()->type)
        {
            case T_LBRACK:
            {
                if (!ast->vtype.arrlen && !ast->vtype.ptr)
                    error("Use of subscript operator '[]' on non-array or pointer type.\n");

                next();

                struct type cpy = ast->vtype;
                if (cpy.arrlen) { cpy.ptr++; cpy.arrlen = 0; }
                struct type ptrd = cpy;
                if (ptrd.ptr) ptrd.ptr--;

                struct ast *binop = mkbinop(OP_PLUS, ast->type == A_UNARY && ast->unary.op == OP_DEREF ? ast->unary.val : ast, mkast(A_SCALE), cpy);
                binop->binop.rhs->scale.val = pre();
                binop->binop.rhs->scale.num = asm_sizeof(ptrd);

                struct ast *access = mkunary(OP_DEREF, binop, ptrd);
                expect(T_RBRACK);
                ast = access;
                break;
            }
            case T_LPAREN:
            {
                if (ast->vtype.name != TYPE_FUNC)
                    error("Call of non-function or function-pointer type.\n");

                struct ast *call = mkast(A_CALL);

                if (ast->vtype.ptr)
                    call->call.ast = ast;
                else
                    call->call.ast = mkunary(OP_ADDROF, ast, ast->vtype);

                call->vtype = *ast->vtype.func.ret;

                next();
                unsigned int i;
                for (i = 0; curr()->type != T_RPAREN; i++)
                {
                    call->call.params = realloc(call->call.params, (ast->call.paramcnt + 1) * sizeof(struct ast*));
                    call->call.params[call->call.paramcnt++] = binexpr();

                    if (curr()->type != T_RPAREN) expect(T_COMMA);
                }
                expect(T_RPAREN);

                if (i < ast->vtype.func.paramcnt)
                    error("Too few parameters in call to function\n");
                else if (i > ast->vtype.func.paramcnt && !ast->vtype.func.variadic)
                    error("Too many parameters in call to function\n");

                ast = call;
                break;
            }
            case T_DOT:
            case T_ARROW: ast = memaccess(ast); break;

            case T_INC:
            case T_DEC:
            {
                struct ast *inc = mkast(curr()->type == T_INC ? A_POSTINC : A_POSTDEC);
                next();
                inc->vtype = ast->vtype;
                inc->incdec.val = ast;
                ast = inc;
                break;
            }

            default: return ast;
        }
    }
}

static struct ast *intlit()
{
    return mkintlit(curr()->v.ival, curr()->v.ival < UINT32_MAX ? mktype(TYPE_UINT32, 0, 0) : mktype(TYPE_UINT64, 0, 0));
}

static struct ast *primary()
{
    struct ast *ast;

    switch (curr()->type)
    {
        case T_SIZEOF:
            next();
            ast = mkast(A_SIZEOF);
            ast->vtype = mktype(TYPE_UINT64, 0, 0);
            ast->sizeofop.t = parsetype();
            return ast;
        case T_INTLIT:
            ast = intlit();
            next();
            return ast;
        case T_STRLIT:
            ast = mkast(A_STRLIT);
            ast->vtype = mktype(TYPE_INT8, 0, 1); // int8*
            ast->strlit.idx = UINT32_MAX;

            for (unsigned int i = 0; i < s_parser.globlscope->block.strcnt; i++)
            {
                if (!strcmp(s_parser.globlscope->block.strs[i].val, curr()->v.sval))
                {
                    ast->strlit.idx = i;
                    break;
                }
            }

            if (ast->strlit.idx == UINT32_MAX)
            {
                ast->strlit.idx = s_parser.globlscope->block.strcnt;

                s_parser.globlscope->block.strs = realloc(s_parser.globlscope->block.strs, (s_parser.globlscope->block.strcnt + 1) * sizeof(struct rostr));
                s_parser.globlscope->block.strs[s_parser.globlscope->block.strcnt++] = (struct rostr)
                {
                    .val = strdup(curr()->v.sval),
                    .lbl = 0
                };
            }

            next();
            return ast;
        case T_LPAREN: return post(parenexpr());

        case T_IDENT:
        {
            char *name = curr()->v.sval;

            struct sym *sym = sym_lookup(s_parser.currscope, name);
            if (!sym)
                error("Use of undeclared symbol '%s'\n", name);
            
            next();
            ast             = mkast(A_IDENT);
            ast->vtype      = sym->type;
            ast->ident.name = strdup(name);
            return ast;
        }

        default:
            error("Expected primary expression, got '%s'\n", tokstrs[curr()->type]);
    }

    return NULL;
}

static int rightassoc(int op)
{
    return op >= OP_ASSIGN && op <= OP_BITOREQ;
}

static int opprec[] =
{
    [OP_MUL] = 3,
    [OP_DIV] = 3,
    [OP_MOD] = 3,
    [OP_PLUS] = 4,
    [OP_MINUS] = 4,
    [OP_SHL] = 5,
    [OP_SHR] = 5,
    [OP_LT] = 6,
    [OP_LTE] = 6,
    [OP_GT] = 6,
    [OP_GTE] = 6,
    [OP_EQUAL] = 7,
    [OP_NEQUAL] = 7,
    [OP_BITAND] = 8,
    [OP_BITXOR] = 9,
    [OP_BITOR] = 10,
    [OP_LAND] = 11,
    [OP_LOR] = 12,
    [OP_TERNARY] = 13,
    [OP_ASSIGN] = 14,
    [OP_PLUSEQ] = 14,
    [OP_MINUSEQ] = 14,
    [OP_MULEQ] = 14,
    [OP_DIVEQ] = 14,
    [OP_MODEQ] = 14,
    [OP_SHLEQ] = 14,
    [OP_SHREQ] = 14,
    [OP_BITANDEQ] = 14,
    [OP_BITXOREQ] = 14,
    [OP_BITOREQ] = 14,
};

static struct ast *recurse_binexpr(int ptp)
{
    struct ast *rhs;
    struct ast *lhs = pre();

    if (termin()) return lhs;

    int op;
    if ((op = operator(curr()->type)) == -1)
        error("Expected operator, got '%s'\n", tokstrs[curr()->type]);

    while (opprec[op] < ptp || (rightassoc(op) && opprec[op] == ptp))
    {
        next();
        rhs = recurse_binexpr(opprec[op]);
        
        if (op == OP_TERNARY)
        {
            expect(T_COLON);
            struct ast *ternary   = mkast(A_TERNARY);
            ternary->ternary.lhs  = rhs;
            ternary->ternary.cond = lhs;
            ternary->ternary.rhs  = binexpr();
            return ternary;
        }

        struct ast *expr = mkbinop(op, lhs, rhs, lhs->vtype);

        if (!type_compatible(lhs->vtype, rhs->vtype))
            error("Incompatible types in binary expression.\n");

        if (op == OP_ASSIGN)
        {
            if (!(lhs->type == A_UNARY && lhs->unary.op == OP_DEREF) && lhs->type != A_IDENT)
                error("Expected an lvalue in assignment.\n");

            lhs->lvalue = 1;
            rhs->lvalue = 0;
        }

        lhs = expr;
        if (termin()) return lhs;

        if ((op = operator(curr()->type)) == -1)
            error("Expected operator, got '%s'\n", tokstrs[curr()->type]);
    }

    return lhs;
}

static struct ast *binexpr()
{
    return fold(recurse_binexpr(15));
}

static struct ast *inlineasm()
{
    struct ast *ast = mkast(A_ASM);
    ast->inasm.code = strdup(curr()->v.sval);
    next();
    return ast;
}

static struct ast *block();
static struct ast *vardecl();

static struct ast *funcdecl()
{
    expect(T_FUNC);

    struct sym sym = { 0 };
    sym.type.name = TYPE_FUNC;
    sym.type.func.ret = calloc(1, sizeof(struct type));

    if (curr()->type == T_PUBLIC)
    {
        sym.attr |= SYM_PUBLIC;
        next();
    }

    if (curr()->type == T_EXTERN)
    {
        sym.attr |= SYM_EXTERN;
        next();
    }

    struct ast *ast = mkast(A_FUNCDEF);
    ast->funcdef.name = strdup(curr()->v.sval);
    sym.name = strdup(ast->funcdef.name);    

    expect(T_IDENT);
    expect(T_LPAREN);

    while (curr()->type != T_RPAREN)
    {
        if (curr()->type == T_ELLIPSIS)
        {
            sym.type.func.variadic = 1;
            next();
            break;
        }

        if (curr()->type == T_IDENT)
        {
            ast->funcdef.params[sym.type.func.paramcnt] = strdup(curr()->v.sval);
            next();
            expect(T_COLON);
        }
        
        sym.type.func.params[sym.type.func.paramcnt] = calloc(1, sizeof(struct type));
        *sym.type.func.params[sym.type.func.paramcnt++] = parsetype();
        if (curr()->type != T_RPAREN) expect(T_COMMA);
    }
    
    expect(T_RPAREN);

    if (curr()->type == T_ARROW)
    {
        next();
        *sym.type.func.ret = parsetype();
    }
    else *sym.type.func.ret = (struct type) { .name = TYPE_VOID };

    struct sym *prev = sym_lookup(s_parser.currscope, sym.name);
    if (prev)
    {
        if (!(sym.attr & SYM_EXTERN) && !(prev->attr & SYM_EXTERN))
            error("Multiple definition of function '%s'\n", sym.name);
    }

    sym_putglob(s_parser.currscope, &sym);

    if (curr()->type == T_LBRACE)
    {
        if (sym.attr & SYM_EXTERN)
            error("Definition of function marked 'extern'.\n");

        s_parser.currfunc = ast;
        
        next();
        ast->funcdef.block = mkast(A_BLOCK);
        ast->funcdef.block->block.symtab.type = SYMTAB_FUNC;

        for (unsigned int i = 0; i < sym.type.func.paramcnt; i++)
            sym_put(&ast->funcdef.block->block.symtab, ast->funcdef.params[i], *sym.type.func.params[i], 0);
        
        block(ast->funcdef.block, SYMTAB_FUNC);
        expect(T_RBRACE);
    
        s_parser.currfunc = NULL;
    }
    else
    {
        if (!(sym.attr & SYM_EXTERN))
            error("No definition of function not marked 'extern'\n");

        expect(T_SEMI);
        free(ast);
        return NULL;
    }

    return ast;
}

static struct ast *vardecl()
{
    int attr = 0;
    if (curr()->type == T_PUBLIC)
    {
        attr |= SYM_PUBLIC;
        next();
    }

    if (curr()->type == T_EXTERN)
    {
        attr |= SYM_EXTERN;
        next();
    }

    const char *name = curr()->v.sval;
    struct sym *prev = sym_lookup(s_parser.currscope, name);
    if (prev && !(prev->attr & SYM_EXTERN) && !(attr & SYM_EXTERN))
        error("Multiple definition of variable '%s'\n", name);

    expect(T_IDENT);
    struct type t;
    struct ast *ast;
    int autov = 0; // Auto type

    if (curr()->type != T_COLON)
    {
        if (curr()->type != T_EQ)
            error("Auto variable must be initialized.\n");
        autov = 1;
    }
    else
    {
        expect(T_COLON);
        t = parsetype();
        ast = mkast(A_VARDEF);
    }

    if (curr()->type == T_EQ)
    {
        expect(T_EQ);
        struct ast *init = binexpr();

        if (autov)
            t = init->vtype;
        else
        {
            if (!type_compatible(ast->binop.rhs->vtype, t))
                error("Incompatible types in variable initialization\n");
        }

        ast = mkbinop(OP_ASSIGN, mkast(A_IDENT), init, t);
        ast->binop.lhs->ident.name = strdup(name);

        ast->binop.rhs->lvalue = 0;
        ast->binop.lhs->lvalue = 1;
    }

    sym_put(s_parser.currscope, name, t, attr);
    return ast;
}

static struct type parse_struct()
{
    struct type struc = mktype(TYPE_STRUCT, 0, 0);
    size_t offset = 0;
    while (curr()->type != T_RBRACE)
    {
        const char *memname = curr()->v.sval;
        expect(T_IDENT);
        
        expect(T_COLON);
        struct type type = parsetype();

        struc.struc.members = realloc(struc.struc.members, (struc.struc.memcnt + 1) * sizeof(struct structmem));
        struc.struc.members[struc.struc.memcnt++] = (struct structmem)
        {
            .name = strdup(memname),
            .type = type,
            .offset = offset
        };

        offset += asm_sizeof(type);

        if (curr()->type != T_RBRACE)
            expect(T_COMMA);
    }

    struc.struc.size = offset;
    return struc;
}

static struct type parse_union()
{
    struct type uni = mktype(TYPE_UNION, 0, 0);
    size_t size = 0;

    while (curr()->type != T_RBRACE)
    {
        const char *memname = curr()->v.sval;
        expect(T_IDENT);
        
        expect(T_COLON);
        struct type type = parsetype();

        uni.struc.members = realloc(uni.struc.members, (uni.struc.memcnt + 1) * sizeof(struct structmem));
        uni.struc.members[uni.struc.memcnt++] = (struct structmem)
        {
            .name = strdup(memname),
            .type = type,
            .offset = 0
        };

        if (asm_sizeof(type) > size) size = asm_sizeof(type);

        if (curr()->type != T_RBRACE)
            expect(T_COMMA);
    }

    uni.struc.size = size;
    return uni;
}

static struct ast *comp_declaration()
{
    struct type type;
    
    int isunion = curr()->type == T_UNION;
    next();

    const char *name = curr()->v.sval;
    expect(T_IDENT);
    expect(T_LBRACE);

    if (isunion) type = parse_union();
    else type = parse_struct();

    expect(T_RBRACE);
    expect(T_SEMI);

    add_typedef(name, type);
    return NULL;
}

static struct ast *return_statement()
{
    next();
    struct ast *ast = mkast(A_RETURN);
    ast->ret.func   = s_parser.currfunc;
    
    if (curr()->type != T_SEMI)
    {
        struct sym *sym = sym_lookup(s_parser.currscope, ast->ret.func->funcdef.name);
        struct type t = *sym->type.func.ret;

        if (t.name == TYPE_VOID && !t.ptr)
            error("Returning value from void function.\n");
        
        ast->ret.val = binexpr();

        if (!type_compatible(ast->ret.val->vtype, t))
            error("Incompatible return type in function '%s'.\n", sym->name);
    }

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
    ast->ifelse.ifblock = mkast(A_BLOCK);
    block(ast->ifelse.ifblock, SYMTAB_BLOCK);
    expect(T_RBRACE);

    if (curr()->type == T_ELSE)
    {
        next();
        expect(T_LBRACE);
        ast->ifelse.elseblock = mkast(A_BLOCK);
        block(ast->ifelse.elseblock, SYMTAB_BLOCK);
        expect(T_RBRACE);
    }

    return ast;
}

static struct ast *while_statement()
{
    next();
    expect(T_LPAREN);

    struct ast *ast = mkast(A_WHILE);

    ast->whileloop.cond = binexpr();
    expect(T_RPAREN);

    expect(T_LBRACE);
    ast->whileloop.body = mkast(A_BLOCK);
    block(ast->whileloop.body, SYMTAB_BLOCK);
    expect(T_RBRACE);

    return ast;
}

static struct ast *statement();

static struct ast *for_statement()
{
    next();
    expect(T_LPAREN);

    struct ast *ast = mkast(A_FOR);
    ast->forloop.body = mkast(A_BLOCK);

    ast->forloop.body->block.symtab.type   = SYMTAB_BLOCK;
    ast->forloop.body->block.symtab.parent = s_parser.currscope;

    s_parser.currscope = &ast->forloop.body->block.symtab;

    ast->forloop.init = statement();
    expect(T_SEMI);

    ast->forloop.cond = binexpr();
    expect(T_SEMI);

    ast->forloop.update = statement();
    expect(T_RPAREN);

    // TODO: This is hacky
    s_parser.currscope = s_parser.currscope->parent;

    expect(T_LBRACE);
    block(ast->forloop.body, SYMTAB_BLOCK);
    expect(T_RBRACE);

    return ast;
}

static struct ast *label()
{
    expect(T_LABEL);
    struct ast *ast = mkast(A_LABEL);
    ast->label.name = strdup(curr()->v.sval);
    
    next();
    expect(T_COLON);
    return ast;
}

static struct ast *gotolbl()
{
    expect(T_GOTO);
    struct ast *ast = mkast(A_GOTO);
    ast->gotolbl.label = strdup(curr()->v.sval);

    next();
    return ast;
}

static struct ast *typedef_statement()
{
    expect(T_TYPEDEF);

    const char *name = curr()->v.sval;
    
    expect(T_IDENT);
    expect(T_EQ);

    struct type type = parsetype();

    add_typedef(name, type);

    expect(T_SEMI);
    return NULL;
}

static struct ast *statement()
{
    switch (curr()->type)
    {
        case T_ASM:     return inlineasm();
        case T_FUNC:    return funcdecl();
        case T_VAR: next(); return vardecl(); // TODO: fix this
        case T_RETURN:  return return_statement();
        case T_IF:      return if_statement();
        case T_WHILE:   return while_statement();
        case T_FOR:     return for_statement();
        case T_LABEL:   return label();
        case T_GOTO:    return gotolbl();
        case T_TYPEDEF: return typedef_statement();
        case T_STRUCT:
        case T_UNION:   return comp_declaration();
        default:        return binexpr();
    }
}

static struct ast *block(struct ast *block, int type)
{
    block->block.symtab.type = type;

    block->block.symtab.parent = s_parser.currscope;
    s_parser.currscope = &block->block.symtab;

    while (curr()->type != T_RBRACE && curr()->type != T_END)
    {
        if (curr()->type == T_EOF)
        {
            next(); continue;
        }

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
    struct ast *tree = mkast(A_BLOCK);
    
    s_parser.toks       = toks;
    s_parser.i          = 0;
    s_parser.currscope  = NULL;
    s_parser.globlscope = tree;

    block(tree, SYMTAB_GLOB);
    memcpy(ast, tree, sizeof(struct ast));

    return 0;
}
