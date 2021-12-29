#include <gen.h>
#include <sym.h>
#include <asm.h>

#include <assert.h>
#include <stdlib.h>

#define CMPEXPR(ast) (ast->type == A_BINOP && ((ast->binop.op > OP_LT && ast->binop.op < OP_NEQUAL) || ast->binop.op == OP_LAND || ast->binop.op == OP_LOR))

static const char *regs8[]  = { "%r8b", "%r9b", "%r10b", "%r11b" };
static const char *regs16[] = { "%r8w", "%r9w", "%r10w", "%r11w" };
static const char *regs32[] = { "%r8d", "%r9d", "%r10d", "%r11d" };
static const char *regs64[] = { "%r8",  "%r9",  "%r10",  "%r11" };

static struct symtable *s_currscope = NULL;
static struct ast      *s_globlscope = NULL;

#define REGCNT (sizeof(regs64) / sizeof(regs64[0]))
#define NOREG (-1)

#define DISCARD(expr) { int r = expr; if (r != NOREG) regfree(r); }

static int reglist[REGCNT] = { 0 };

static int label()
{
    static int labels = 0;
    return labels++;
}

// Allocate a register
static int regalloc()
{
    for (unsigned int i = 0; i < REGCNT; i++)
    {
        if (!reglist[i])
        {
            reglist[i] = 1;
            return i;
        }
    }

    printf("Out of registers\n");
    abort();
    return NOREG;
}

static void regfree(int r)
{
    assert(r >= 0 && r < (int)REGCNT);
    reglist[r] = 0;
}

static const char *setinsts[] =
{
    [OP_EQUAL]  = "sete",
    [OP_NEQUAL] = "setne",
    [OP_GT]     = "setg",
    [OP_LT]     = "setl",
    [OP_GTE]    = "setge",
    [OP_LTE]    = "setle"
};

static const char *jmpinsts[] =
{
    [OP_EQUAL]  = "jne",
    [OP_NEQUAL] = "je",
    [OP_GT]     = "jl",
    [OP_LT]     = "jg",
    [OP_GTE]    = "jle",
    [OP_LTE]    = "jge"
};

static int add_string(const char *str, FILE *file)
{
    int l = label();

    fprintf(file, "\t.section .rodata\n");
    fprintf(file, "L%d:\n\t.string \"%s\"\n", l, str);
    fprintf(file, "\t.section .text\n");

    return l;
}

static int asm_addrof(struct sym *sym, int r, FILE *file)
{
    if (sym->attr & SYM_GLOBAL)
        fprintf(file, "\tleaq\t%s(%%rip), %s\n", sym->name, regs64[r]);
    else
        fprintf(file, "\tleaq\t-%lu(%%rbp), %s\n", sym->stackoff, regs64[r]);
    return r;
}

static const char *movs[9] =
{
    [1] = "movb",
    [2] = "movw",
    [4] = "movl",
    [8] = "movq"
};

static const char *movzs[9] =
{
    [1] = "movzbq",
    [2] = "movzwq",
    [4] = "movslq",
    [8] = "movq"
};

static const char **regs[9] =
{
    [1] = regs8,
    [2] = regs16,
    [4] = regs32,
    [8] = regs64
};

static int gen_load(struct sym *sym, int r, FILE *file)
{
    if (sym->type.arrlen)
        return asm_addrof(sym, r, file);
    else
    {
        if (sym->attr & SYM_LOCAL)
            fprintf(file, "\t%s\t-%lu(%%rbp), %s\n", movzs[asm_sizeof(sym->type)], sym->stackoff, regs64[r]);
        else
            fprintf(file, "\t%s\t%s(%%rip), %s\n", movzs[asm_sizeof(sym->type)], sym->name, regs64[r]);
        return r;
    }
}

static int gen_store(struct sym *sym, int r, FILE* file)
{
    if (sym->attr & SYM_LOCAL)
        fprintf(file, "\t%s\t%s, -%lu(%%rbp)\n", movs[asm_sizeof(sym->type)], regs[asm_sizeof(sym->type)][r], sym->stackoff);
    else
        fprintf(file, "\t%s\t%s, %s(%%rip)\n", movs[asm_sizeof(sym->type)], regs[asm_sizeof(sym->type)][r], sym->name);
    return r;
}

static int gen_storederef(struct ast *ast, int r1, int r2, FILE *file)
{
    fprintf(file, "\t%s\t%s, (%s)\n", movs[asm_sizeof(ast->vtype)], regs[asm_sizeof(ast->vtype)][r2], regs64[r1]);
    regfree(r1);
    return r2;
}

static int gen_loadderef(struct ast *ast, int r, FILE *file)
{
    if (ast->lvalue)
        return r;
    else
    {
        int r2 = regalloc();
        fprintf(file, "\t%s\t(%s), %s\n", movs[asm_sizeof(ast->vtype)], regs64[r], regs[asm_sizeof(ast->vtype)][r2]);
        regfree(r);
        return r2;
    }
}

static int gen_add(int r1, int r2, FILE *file)
{
    fprintf(file, "\taddq\t%s, %s\n", regs64[r2], regs64[r1]);
    regfree(r2);
    return r1;
}

static int gen_sub(int r1, int r2, FILE *file)
{
    fprintf(file, "\tsubq\t%s, %s\n", regs64[r2], regs64[r1]);
    regfree(r2);
    return r1;
}

static int gen_mul(int r1, int r2, FILE *file)
{
    fprintf(file, "\timulq\t%s, %s\n", regs64[r1], regs64[r2]);
    regfree(r2);
    return r1;
}

static int gen_div(int r1, int r2, FILE *file)
{
    fprintf(file, "\tcqo\n");
    fprintf(file, "\tmovq\t%s, %%rax\n", regs64[r1]);
    fprintf(file, "\tidiv\t%s\n", regs64[r2]);
    fprintf(file, "\tmovq\t%%rax, %s\n", regs64[r1]);
    regfree(r2);
    return r1;
}

static int gen_mod(int r1, int r2, FILE *file)
{
    fprintf(file, "\tcqo\n");
    fprintf(file, "\tmovq\t%s, %%rax\n", regs64[r1]);
    fprintf(file, "\tidiv\t%s\n", regs64[r2]);
    fprintf(file, "\tmovq\t%%rdx, %s\n", regs64[r1]);
    regfree(r2);
    return r1;
}

static int gen_shl(int r1, int r2, FILE *file)
{
    fprintf(file, "\tmovb\t%s, %%cl\n", regs8[r2]);
    fprintf(file, "\tshlq\t%%cl, %s\n", regs64[r1]);
    regfree(r2);
    return r1;
}

static int gen_shr(int r1, int r2, FILE *file)
{
    fprintf(file, "\tmovb\t%s, %%cl\n", regs8[r2]);
    fprintf(file, "\tshrq\t%%cl, %s\n", regs64[r1]);
    regfree(r2);
    return r1;
}

static int gen_bitand(int r1, int r2, FILE *file)
{
    fprintf(file, "\tandq\t%s, %s\n", regs64[r2], regs64[r1]);
    regfree(r2);
    return r1;
}

static int gen_bitor(int r1, int r2, FILE *file)
{
    fprintf(file, "\torq\t%s, %s\n", regs64[r2], regs64[r1]);
    regfree(r2);
    return r1;
}

static int gen_bitxor(int r1, int r2, FILE *file)
{
    fprintf(file, "\txorq\t%s, %s\n", regs64[r2], regs64[r1]);
    regfree(r2);
    return r1;
}

static int gen_cmpandset(int r1, int r2, int op, FILE *file)
{
    int r = regalloc();
    fprintf(file, "\tcmpq\t%s, %s\n", regs64[r2], regs64[r1]);
    fprintf(file, "\t%s\t%%al\n", setinsts[op]);
    fprintf(file, "\tmovzx\t%%al, %s\n", regs64[r]);
    
    regfree(r1);
    regfree(r2);
    return r;
}

// Compare two registers and jmp past code block
static int gen_cmpandjmp(int r1, int r2, int op, int lbl, FILE *file)
{
    fprintf(file, "\tcmpq\t%s, %s\n", regs64[r2], regs64[r1]);
    fprintf(file, "\t%s\tL%d\n", jmpinsts[op], lbl);
    regfree(r1);
    regfree(r2);
    return NOREG;
}

// Assignment binary expressions e.g. x = 10, x += 2, *x /= 2, etc
static int gen_assign(int r1, int r2, struct ast *ast, FILE *file)
{
    switch (ast->binop.op)
    {
        case OP_PLUSEQ:   r2 = gen_add(r1, r2, file); break;
        case OP_MULEQ:    r2 = gen_mul(r1, r2, file); break;
        case OP_DIVEQ:    r2 = gen_div(r1, r2, file); break;
        case OP_MINUSEQ:  r2 = gen_sub(r1, r2, file); break;
        case OP_BITANDEQ: r2 = gen_bitand(r1, r2, file); break;
        case OP_BITOREQ:  r2 = gen_bitor(r1, r2, file); break;
        case OP_BITXOREQ: r2 = gen_bitxor(r1, r2, file); break;
        case OP_SHLEQ:    r2 = gen_shl(r1, r2, file); break;
        case OP_SHREQ:    r2 = gen_shr(r1, r2, file); break;
        case OP_MODEQ:    r2 = gen_mod(r1, r2, file); break;
    }
    
    if (ast->binop.lhs->type == A_UNARY && ast->binop.lhs->unary.op == OP_DEREF)
        return gen_storederef(ast, r1, r2, file);
    else
    {
        struct sym *sym = sym_lookup(s_currscope, ast->binop.lhs->ident.name);
        return gen_store(sym, r2, file);
    }
}

static int gen_lazyeval(struct ast *ast, FILE *file)
{
    int end = label();

    int r1 = gen_code(ast->binop.lhs, file);

    fprintf(file, "\ttest\t%s, %s\n", regs64[r1], regs64[r1]);

    if (ast->binop.op == OP_LAND)
        fprintf(file, "\tjz\tL%d\n", end);
    else if (ast->binop.op == OP_LOR)
        fprintf(file, "\tjnz\tL%d\n", end);

    if (!CMPEXPR(ast->binop.lhs))
    {
        fprintf(file, "\tsetne\t%s\n", regs8[r1]);
        fprintf(file, "\tmovzbq\t%s, %s\n", regs8[r1], regs64[r1]);
    }

    int r2 = gen_code(ast->binop.rhs, file);
    if (!CMPEXPR(ast->binop.rhs))
    {
        fprintf(file, "\ttest\t%s, %s\n", regs64[r2], regs64[r2]);
        fprintf(file, "\tsetne\t%s\n", regs8[r2]);
        fprintf(file, "\tmovzbq\t%s, %s\n", regs8[r2], regs64[r2]);
    }

    regfree(r1);
    fprintf(file, "L%d:\n", end);
    return r2;
}

static int gen_lazyevaljmp(int lbl, struct ast *ast, FILE *file)
{
    int r1 = gen_code(ast->binop.lhs, file);

    fprintf(file, "\ttest\t%s, %s\n", regs64[r1], regs64[r1]);
    regfree(r1);
    
    if (ast->binop.op == OP_LAND)
        fprintf(file, "\tjz\tL%d\n", lbl);
    else if (ast->binop.op == OP_LOR)
        fprintf(file, "\tjnz\tL%d\n", lbl);

    int r2 = gen_code(ast->binop.rhs, file);
    fprintf(file, "\ttest\t%s, %s\n", regs64[r2], regs64[r2]);
    fprintf(file, "\tjnz\tL%d\n", lbl);
    regfree(r2);
    return NOREG;
}

static int gen_binop(struct ast *ast, FILE *file)
{
    if (ast->binop.op == OP_LAND || ast->binop.op == OP_LOR)
        return gen_lazyeval(ast, file);

    int r1 = gen_code(ast->binop.lhs, file);
    int r2 = gen_code(ast->binop.rhs, file);

    switch (ast->binop.op)
    {
        case OP_PLUS:   return gen_add(r1, r2, file); 
        case OP_MUL:    return gen_mul(r1, r2, file);
        case OP_DIV:    return gen_div(r1, r2, file);      
        case OP_MINUS:  return gen_sub(r1, r2, file);
        case OP_SHL:    return gen_shl(r1, r2, file);
        case OP_SHR:    return gen_shr(r1, r2, file);
        case OP_MOD:    return gen_mod(r1, r2, file);
        case OP_EQUAL:
        case OP_NEQUAL:
        case OP_GT:
        case OP_LT:
        case OP_GTE:
        case OP_LTE:    return gen_cmpandset(r1, r2, ast->binop.op, file);
        case OP_ASSIGN:
        case OP_PLUSEQ:
        case OP_MINUSEQ:
        case OP_MULEQ:
        case OP_DIVEQ:
        case OP_BITANDEQ:
        case OP_BITOREQ:
        case OP_BITXOREQ:
        case OP_SHLEQ:
        case OP_SHREQ:
        case OP_MODEQ:  return gen_assign(r1, r2, ast, file);
        case OP_BITAND: return gen_bitand(r1, r2, file);
        case OP_BITOR:  return gen_bitor(r1, r2, file);
        case OP_BITXOR: return gen_bitxor(r1, r2, file);
    }

    return NOREG;
}

static int gen_lognot(int r, FILE *file)
{
    fprintf(file, "\ttest\t%s, %s\n", regs64[r], regs64[r]);
    fprintf(file, "\tsete\t%s\n", regs8[r]);
    fprintf(file, "\tmovzbq\t%s, %s\n", regs8[r], regs64[r]);
    return r;
}

static int gen_bitnot(int r, FILE *file)
{
    fprintf(file, "\tnotq\t%s\n", regs64[r]);
    return r;
}

static int gen_unaryminus(int r, FILE *file)
{
    fprintf(file, "\tnegq\t%s\n", regs64[r]);
    return r;
}

static int gen_addrof(struct ast *ast, FILE *file)
{
    int r = regalloc();
    return asm_addrof(sym_lookup(s_currscope, ast->unary.val->ident.name), r, file);
}

static int gen_unary(struct ast *ast, FILE *file)
{
    if (ast->unary.op == OP_ADDROF)
        return gen_addrof(ast, file);

    int r = gen_code(ast->unary.val, file);

    switch (ast->unary.op)
    {
        case OP_DEREF:  return gen_loadderef(ast, r, file); break;
        case OP_LOGNOT: return gen_lognot(r, file); break;
        case OP_BITNOT: return gen_bitnot(r, file); break;
        case OP_MINUS:  return gen_unaryminus(r, file); break;
    }

    return NOREG;
}

static int gen_intlit(struct ast *ast, FILE *file)
{
    int r = regalloc();
    fprintf(file, "\tmovq\t$%ld, %s\n", ast->intlit.ival, regs64[r]);
    return r;
}

static int gen_ident(struct ast *ast, FILE *file)
{
    if (ast->lvalue) return NOREG;

    int r = regalloc();
    struct sym *sym = sym_lookup(s_currscope, ast->ident.name);
    return gen_load(sym, r, file);
}

static int gen_block(struct ast *ast, FILE *file)
{
    s_currscope = &ast->block.symtab;

    for (unsigned int i = 0; i < ast->block.cnt; i++)
        DISCARD(gen_code(ast->block.statements[i], file));

    s_currscope = s_currscope->parent;
    return NOREG;
}

static const char *paramregs64[6] =
{
    "%rdi", "%rsi", "%rdx", "%rcx", "%r8", "%r9"
};

static const char *paramregs32[6] =
{
    "%edi", "%esi", "%edx", "%ecx", "%r8d", "%r9d"
};

static const char *paramregs16[6] =
{
    "%di", "%si", "%dx", "%cx", "%r8w", "%r9w"
};

static const char *paramregs8[6] =
{
    "%dil", "%sil", "%dl", "%cl", "%r8b", "%r9b"
};

static const char **paramregs[] =
{
    [1] = paramregs8,
    [2] = paramregs16,
    [4] = paramregs32,
    [8] = paramregs64
};

static int gen_funcdef(struct ast *ast, FILE *file)
{
    ast->funcdef.endlbl = label();

    struct sym *sym = sym_lookup(s_currscope, ast->funcdef.name);
    if (sym->attr & SYM_PUBLIC)
        fprintf(file, "\t.global %s\n", ast->funcdef.name);
    
    fprintf(file, "%s:\n", ast->funcdef.name);

    if (ast->funcdef.block->block.symtab.curr_stackoff)
    {
        fprintf(file, "\tpushq\t%%rbp\n");
        fprintf(file, "\tmovq\t%%rsp, %%rbp\n");
        fprintf(file, "\tsubq\t$%lu, %%rsp\n", ast->funcdef.block->block.symtab.curr_stackoff);
    }

    for (unsigned int i = 0; i < sym->type.func.paramcnt; i++)
    {
        struct sym *sym = sym_lookup(&ast->funcdef.block->block.symtab, ast->funcdef.params[i]);
        size_t size = asm_sizeof(sym->type);
        fprintf(file, "\t%s\t%s, -%lu(%%rbp)\n", movs[size], paramregs[size][i], sym->stackoff);
    }

    gen_block(ast->funcdef.block, file);
    fprintf(file, "L%d:\n", ast->funcdef.endlbl);

    if (ast->funcdef.block->block.symtab.curr_stackoff)
        fprintf(file, "\tleaveq\n");
    
    fprintf(file, "\tretq\n\n");
    return NOREG;
}

static int gen_inlineasm(struct ast *ast, FILE *file)
{
    fprintf(file, "%s", ast->inasm.code);
    return NOREG;
}

static int gen_call(struct ast *ast, FILE *file)
{
    for (unsigned int i = 0; i < ast->call.paramcnt; i++)
    {
        int par = gen_code(ast->call.params[i], file);
        size_t s = asm_sizeof(ast->call.params[i]->vtype);
        fprintf(file, "\t%s\t%s, %s\n", movs[s], regs[s][par], paramregs[s][i]);
        regfree(par);
    }

    const char *fn;
    if (ast->call.ast->type == A_UNARY && ast->call.ast->unary.op == OP_ADDROF
        && ast->call.ast->unary.val->type == A_IDENT)
        fn = ast->call.ast->unary.val->ident.name;
    else
    {
        int r = gen_code(ast->call.ast, file);
        fn = regs64[r];
        regfree(r); // A bit dodgy, makes for better looking code
    }
    
    if (ast->call.ast->vtype.func.variadic)
        fprintf(file, "\txorq\t%%rax, %%rax\n");
    fprintf(file, "\tcallq\t%s\n", fn);

    if (ast->vtype.name == TYPE_VOID && !ast->vtype.ptr)
        return NOREG;

    int r = regalloc();
    fprintf(file, "\tmovq\t%%rax, %s\n", regs64[r]);
    return r;
}

static const char *retrs[] =
{
    [1] = "%al",
    [2] = "%ax",
    [4] = "%eax",
    [8] = "%rax"
};

static int gen_return(struct ast *ast, FILE *file)
{
    if (ast->ret.val)
    {
        int r = gen_code(ast->ret.val, file);
        size_t s = asm_sizeof(ast->ret.val->vtype);
        fprintf(file, "\t%s\t%s, %s\n", movs[s], regs[s][r], retrs[s]);
        regfree(r);
    }

    fprintf(file, "\tjmp\tL%d\n", ast->ret.func->funcdef.endlbl);
    return NOREG;
}

static int gen_ifelse(struct ast *ast, FILE *file)
{
    int elselbl = -1;
    if (ast->ifelse.elseblock) elselbl = label();
    
    int endlbl = label();

    struct ast *cond = ast->ifelse.cond;
    if (CMPEXPR(cond))
    {
        if (cond->binop.op == OP_LAND || cond->binop.op == OP_LOR)
            gen_lazyevaljmp(elselbl != -1 ? elselbl : endlbl, cond, file);
        else
        {
            int r1 = gen_code(cond->binop.lhs, file);
            int r2 = gen_code(cond->binop.rhs, file);
            gen_cmpandjmp(r1, r2, cond->binop.op, elselbl != -1 ? elselbl : endlbl, file);
        }
    }
    else
    {
        int r = gen_code(cond, file);
        fprintf(file, "\tcmpq\t$0, %s\n", regs64[r]);
        fprintf(file, "\tjz\tL%d\n", elselbl != -1 ? elselbl : endlbl);
        regfree(r);
    }

    DISCARD(gen_code(ast->ifelse.ifblock, file));
    
    if (elselbl != -1)
    {
        fprintf(file, "\tjmp\tL%d\n", endlbl);
        fprintf(file, "L%d:\n", elselbl);
        DISCARD(gen_code(ast->ifelse.elseblock, file));
    }

    fprintf(file, "L%d:\n", endlbl);
    return NOREG;
}

static int gen_while(struct ast *ast, FILE *file)
{
    int looplbl = label(), endlbl = label();

    fprintf(file, "L%d:\n", looplbl);
    int r = gen_code(ast->ifelse.cond, file);
    
    fprintf(file, "\tcmpq\t$0, %s\n", regs64[r]);
    fprintf(file, "\tjz\tL%d\n", endlbl);

    regfree(r);

    DISCARD(gen_code(ast->whileloop.body, file));

    fprintf(file, "\tjmp\tL%d\n", looplbl);

    fprintf(file, "L%d:\n", endlbl);
    return NOREG;
}

// TODO: refactor block/symtab stuff, because the code that follows is very ugly and hacky

static int gen_for(struct ast *ast, FILE *file)
{
    int looplbl = label(), endlbl = label();

    s_currscope = &ast->forloop.body->block.symtab;
    DISCARD(gen_code(ast->forloop.init, file));

    fprintf(file, "L%d:\n", looplbl);
    int r = gen_code(ast->forloop.cond, file);
    
    fprintf(file, "\tcmpq\t$0, %s\n", regs64[r]);
    fprintf(file, "\tjz\tL%d\n", endlbl);

    regfree(r);

    DISCARD(gen_code(ast->forloop.body, file));
    // TODO: this is hacky
    s_currscope = &ast->forloop.body->block.symtab;
    DISCARD(gen_code(ast->forloop.update, file));

    s_currscope = ast->forloop.body->block.symtab.parent;
    fprintf(file, "\tjmp\tL%d\n", looplbl);
    fprintf(file, "L%d:\n", endlbl);
    return NOREG;
}

static int gen_strlit(struct ast *ast, FILE *file)
{
    int r = regalloc();
    fprintf(file, "\tleaq\tL%d(%%rip), %s\n", s_globlscope->block.strs[ast->strlit.idx].lbl, regs64[r]);
    return r;
}

static int gen_sizeof(struct ast *ast, FILE *file)
{
    int r = regalloc();
    fprintf(file, "\tmovq\t$%lu, %s\n", asm_sizeof(ast->sizeofop.t), regs64[r]);
    return r;
}

int gen_label(struct ast *ast, FILE *file)
{
    fprintf(file, "%s:\n", ast->label.name);
    return NOREG;
}

int gen_goto(struct ast *ast, FILE *file)
{
    fprintf(file, "\tjmp\t%s\n", ast->gotolbl.label);
    return NOREG;
}

int gen_cast(struct ast *ast, FILE *file)
{
    return gen_code(ast->cast.val, file);
}

// Pre-increment or decrement
int gen_pre(struct ast *ast, FILE *file)
{
    const char *inst = ast->type == A_PREINC ? "incq" : "decq";

    int r1 = gen_code(ast->incdec.val, file);
    if (ast->incdec.val->type == A_UNARY && ast->incdec.val->unary.op == OP_DEREF)
    {
        int r2 = gen_code(ast->incdec.val->unary.val, file);
        
        fprintf(file, "\t%s\t%s\n", inst, regs64[r1]);
        return gen_storederef(ast, r2, r1, file);
    }
    else
    {
        fprintf(file, "\t%s\t%s\n", inst, regs64[r1]);
        struct sym *sym = sym_lookup(s_currscope, ast->incdec.val->ident.name);
        return gen_store(sym, r1, file);
    }
}

int gen_post(struct ast *ast, FILE *file)
{
    const char *inst = ast->type == A_POSTINC ? "incq" : "decq";

    int r1 = gen_code(ast->incdec.val, file);
    if (ast->incdec.val->type == A_UNARY && ast->incdec.val->unary.op == OP_DEREF)
    {
        int r2 = gen_code(ast->incdec.val->unary.val, file);
        int r3 = regalloc();

        fprintf(file, "\tmovq\t%s, %s\n", regs64[r1], regs64[r3]);
        fprintf(file, "\t%s\t%s\n", inst, regs64[r3]);

        int r = gen_storederef(ast, r2, r3, file);
        regfree(r2);
        return r;
    }
    else
    {
        int r2 = regalloc();

        fprintf(file, "\tmovq\t%s, %s\n", regs64[r1], regs64[r2]);
        fprintf(file, "\t%s\t%s\n", inst, regs64[r2]);
        
        struct sym *sym = sym_lookup(s_currscope, ast->incdec.val->ident.name);
        gen_store(sym, r2, file);
        return r1;
    }
}

int gen_scale(struct ast *ast, FILE *file)
{
    int r = gen_code(ast->scale.val, file);
    fprintf(file, "\timulq\t$%u, %s\n", ast->scale.num, regs64[r]);
    return r;
}

int gen_ternary(struct ast *ast, FILE *file)
{
    int l1 = label(), l2 = label();
    int r1 = gen_code(ast->ternary.cond, file);
    int r2 = regalloc();

    fprintf(file, "\tcmpq\t$0, %s\n", regs64[r1]);
    fprintf(file, "\tjz\tL%d\n", l1);
    regfree(r1);

    int r3 = gen_code(ast->ternary.lhs, file);
    fprintf(file, "\tmovq\t%s, %s\n", regs64[r3], regs64[r2]);
    regfree(r3);

    fprintf(file, "\tjmp\tL%d\n", l2);
    fprintf(file, "L%d:\n", l1);
    
    r3 = gen_code(ast->ternary.rhs, file);
    fprintf(file, "\tmovq\t%s, %s\n", regs64[r3], regs64[r2]);
    regfree(r3);

    fprintf(file, "L%d:\n", l2);
    return r2;
}

// Generate code for an AST node
int gen_code(struct ast *ast, FILE *file)
{
    switch (ast->type)
    {
        case A_BINOP:   return gen_binop(ast, file);
        case A_UNARY:   return gen_unary(ast, file);
        case A_INTLIT:  return gen_intlit(ast, file);
        case A_CALL:    return gen_call(ast, file);
        case A_IDENT:   return gen_ident(ast, file);
        case A_STRLIT:  return gen_strlit(ast, file);
        case A_SIZEOF:  return gen_sizeof(ast, file);
        case A_CAST:    return gen_cast(ast, file);
        case A_PREINC:
        case A_PREDEC:  return gen_pre(ast, file);
        case A_POSTINC:
        case A_POSTDEC: return gen_post(ast, file);
        case A_SCALE:   return gen_scale(ast, file);
        case A_FUNCDEF: return gen_funcdef(ast, file);
        case A_ASM:     return gen_inlineasm(ast, file);
        case A_BLOCK:   return gen_block(ast, file);
        case A_RETURN:  return gen_return(ast, file);
        case A_IFELSE:  return gen_ifelse(ast, file);
        case A_WHILE:   return gen_while(ast, file);
        case A_FOR:     return gen_for(ast, file);
        case A_LABEL:   return gen_label(ast, file);
        case A_GOTO:    return gen_goto(ast, file);
        case A_TERNARY: return gen_ternary(ast, file);
    }

    return NOREG;
}

void gen_datavar(struct type t, FILE *file)
{
    if (t.arrlen)
    {
        struct type base = t;
        base.arrlen = 0;

        for (int i = 0; i < t.arrlen; i++)
            gen_datavar(base, file);
    }
    else if (t.name == TYPE_STRUCT)
    {
        for (unsigned int i = 0; i < t.struc.memcnt; i++)
            gen_datavar(t.struc.members[i].type, file);
    }
    else if (t.ptr)
        fprintf(file, "\t.long 0\n");
    else
    {
        switch (asm_sizeof(t))
        {
            case 1: fprintf(file, "\t.byte  0\n"); break;
            case 2: fprintf(file, "\t.short 0\n"); break;
            case 4: fprintf(file, "\t.int   0\n"); break;
            case 8: fprintf(file, "\t.long  0\n"); break;
        }
    }
}

void gen_ast(struct ast *ast, FILE *file)
{
    s_globlscope = ast;

    fprintf(file, "\t.section .rodata\n");

    for (unsigned int i = 0; i < ast->block.strcnt; i++)
        fprintf(file, "L%d: .string \"%s\"\n", ast->block.strs[i].lbl = label(), ast->block.strs[i].val);

    fprintf(file, "\t.section .data\n");

    for (unsigned int i = 0; i < ast->block.symtab.cnt; i++)
    {
        struct sym *sym = &ast->block.symtab.syms[i];
        if (sym->attr & SYM_GLOBAL && !(sym->type.name == TYPE_FUNC && !sym->type.ptr))
        {
            if (sym->attr & SYM_PUBLIC)
                fprintf(file, "\t.global %s\n", sym->name);
            fprintf(file, "%s:\n", sym->name);
            
            gen_datavar(sym->type, file); 
        }
    }

    fprintf(file, "\t.section .text\n");
    gen_code(ast, file);
}