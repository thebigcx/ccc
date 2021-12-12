#include <gen.h>
#include <sym.h>
#include <asm.h>

#include <assert.h>
#include <stdlib.h>

/*static const char *regs8[]  = { "%r8b", "%r9b", "%r10b", "%r11b" };
static const char *regs16[] = { "%r8w", "%r9w", "%r10w", "%r11w" };
static const char *regs32[] = { "%r8d", "%r9d", "%r10d", "%r11d" };*/
static const char *regs64[] = { "%r8",  "%r9",  "%r10",  "%r11" };

static struct symtable *s_currscope = NULL;

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

static const char *set_instructions[] =
{
    [OP_EQUAL]  = "sete",
    [OP_NEQUAL] = "setne",
    [OP_GT]     = "setg",
    [OP_LT]     = "setl",
    [OP_GTE]    = "setge",
    [OP_LTE]    = "setle"
};

static int add_string(const char *str, FILE *file)
{
    int l = label();

    fprintf(file, "\t.section .rodata\n");
    fprintf(file, "L%d:\n\t.string \"%s\"\n", l, str);
    fprintf(file, "\t.section .text\n");

    return l;
}

static int asm_addrof(struct sym *sym, FILE *file)
{
    int r = regalloc();

    if (sym->attr & SYM_GLOBAL)
        fprintf(file, "\tlea\t%s(%%rip), %s\n", sym->name, regs64[r]);
    else
        fprintf(file, "\tlea\t-%lu(%%rsp), %s\n", sym->stackoff, regs64[r]);

    return r;
}

// gen_* functions return the register

static int gen_binop(struct ast *ast, FILE *file)
{
    int r1 = gen_code(ast->binop.lhs, file);
    int r2 = gen_code(ast->binop.rhs, file);

    switch (ast->binop.op)
    {
        case OP_PLUS:        
            fprintf(file, "\tadd\t%s, %s\n", regs64[r1], regs64[r2]);
            break;
        case OP_MINUS:
            fprintf(file, "\tsub\t%s, %s\n", regs64[r1], regs64[r2]);
            break;
        case OP_MUL:
            fprintf(file, "\timul\t%s, %s\n", regs64[r1], regs64[r2]);
            break;
        case OP_DIV: break;

        case OP_EQUAL:
        case OP_NEQUAL:
        case OP_GT:
        case OP_LT:
        case OP_GTE:
        case OP_LTE:
        {
            int r = regalloc();
            fprintf(file, "\tcmp\t%s, %s\n", regs64[r1], regs64[r2]);
            fprintf(file, "\t%s\t%%al\n", set_instructions[ast->binop.op]);
            fprintf(file, "\tmovzx\t%%al, %s\n", regs64[r]);
            
            regfree(r1);
            regfree(r2);
            return r;
        }
        case OP_ASSIGN:
        {
            if (ast->binop.lhs->type == A_UNARY && ast->binop.lhs->unary.op == OP_DEREF)
                fprintf(file, "\tmov\t%s, (%s)\n", regs64[r2], regs64[r1]);
            else
            {
                struct sym *sym = sym_lookup(s_currscope, ast->binop.lhs->ident.name);
                if (sym->attr & SYM_LOCAL)
                    fprintf(file, "\tmov\t%s, -%lu(%%rbp)\n", regs64[r2], sym->stackoff);
                else
                    fprintf(file, "\tmov\t%s, %s(%%rip)\n", regs64[r2], sym->name);
            }
            
            break;
        }
    }

    regfree(r1);
    return r2;
}

static int gen_unary(struct ast *ast, FILE *file)
{
    switch (ast->unary.op)
    {
        case OP_ADDROF:
        {
            return asm_addrof(sym_lookup(s_currscope, ast->unary.val->ident.name), file);
        }
        case OP_DEREF:
        {
            int r1 = gen_code(ast->unary.val, file);
            if (ast->lvalue)
                return r1;
            else
            {
                int r2 = regalloc();
                fprintf(file, "\tmov\t(%s), %s\n", regs64[r1], regs64[r2]);
                regfree(r1);
                return r2;
            }
        }
    }

    return NOREG;
}

static int gen_intlit(struct ast *ast, FILE *file)
{
    int r = regalloc();
    fprintf(file, "\tmov\t$%ld, %s\n", ast->intlit.ival, regs64[r]);
    return r;
}

static int gen_ident(struct ast *ast, FILE *file)
{
    int r = regalloc();

    struct sym *sym = sym_lookup(s_currscope, ast->ident.name);
    if (sym->attr & SYM_LOCAL)
        fprintf(file, "\tmov\t-%lu(%%rbp), %s\n", sym->stackoff, regs64[r]);
    else
        fprintf(file, "\tmov\t%s(%%rip), %s\n", sym->name, regs64[r]);
    
    return r;
}

static void gen_block(struct ast *ast, FILE *file)
{
    s_currscope = &ast->block.symtab;

    if (!s_currscope->global)
    {
        fprintf(file, "\tpush\t%%rbp\n");
        fprintf(file, "\tmov\t%%rsp, %%rbp\n");
        fprintf(file, "\tsub\t$%lu, %%rsp\n", s_currscope->curr_stackoff);
    }

    for (unsigned int i = 0; i < ast->block.cnt; i++)
        DISCARD(gen_code(ast->block.statements[i], file));

    if (!s_currscope->global)
    {
        fprintf(file, "\tmov\t%%rbp, %%rsp\n");
        fprintf(file, "\tpop\t%%rbp\n");
    }

    s_currscope = s_currscope->parent;
}

static void gen_funcdef(struct ast *ast, FILE *file)
{
    fprintf(file, "\t.global %s\n", ast->funcdef.name); // TODO: storage class
    fprintf(file, "%s:\n", ast->funcdef.name);
    gen_block(ast->funcdef.block, file);
    fprintf(file, "\tret\n");
}

static void gen_inlineasm(struct ast *ast, FILE *file)
{
    fprintf(file, "%s", ast->inasm.code);
}

static const char *paramregs[6] =
{
    "%rdi", "%rsi", "%rdx", "%rcx", "%r8", "%r9"
};

static int gen_call(struct ast *ast, FILE *file)
{
    for (unsigned int i = 0; i < ast->call.paramcnt; i++)
    {
        int par = gen_code(ast->call.params[i], file);
        fprintf(file, "\tmov\t%s, %s\n", regs64[par], paramregs[i]);
        regfree(par);
    }

    int r = regalloc();
    fprintf(file, "\tcall\t%s\n", ast->call.name);
    fprintf(file, "\tmov\t%%rax, %s\n", regs64[r]);
    return r;
}

static void gen_return(struct ast *ast, FILE *file)
{
    if (ast->ret.val)
    {
        int r = gen_code(ast->ret.val, file);
        fprintf(file, "\tmov\t%s, %%rax\n", regs64[r]);
        regfree(r);
    }
    
    fprintf(file, "\tret\n");
}

static void gen_ifelse(struct ast *ast, FILE *file)
{
    int r = gen_code(ast->ifelse.cond, file);

    int elselbl = -1;
    if (ast->ifelse.elseblock) elselbl = label();
    
    int endlbl = label();

    fprintf(file, "\tmov\t$1, %%rax\n");
    fprintf(file, "\tcmp\t%s, %%rax\n", regs64[r]);
    fprintf(file, "\tjne\tL%d\n", elselbl != -1 ? elselbl : endlbl);

    regfree(r);

    DISCARD(gen_code(ast->ifelse.ifblock, file));
    
    if (elselbl != -1)
    {
        fprintf(file, "\tjmp\tL%d\n", endlbl);
        fprintf(file, "L%d:\n", elselbl);
        DISCARD(gen_code(ast->ifelse.elseblock, file));
    }

    fprintf(file, "L%d:\n", endlbl);
}

static void gen_while(struct ast *ast, FILE *file)
{
    int looplbl = label(), endlbl = label();

    fprintf(file, "L%d:\n", looplbl);
    int r = gen_code(ast->ifelse.cond, file);
    
    fprintf(file, "\tmov\t$1, %%rax\n");
    fprintf(file, "\tcmp\t%s, %%rax\n", regs64[r]);
    fprintf(file, "\tjne\tL%d\n", endlbl);

    regfree(r);

    DISCARD(gen_code(ast->whileloop.body, file));

    fprintf(file, "\tjmp\tL%d\n", looplbl);

    fprintf(file, "L%d:\n", endlbl);
}

static void gen_for(struct ast *ast, FILE *file)
{
    int looplbl = label(), endlbl = label();

    DISCARD(gen_code(ast->forloop.init, file));

    fprintf(file, "L%d:\n", looplbl);
    int r = gen_code(ast->forloop.cond, file);
    
    fprintf(file, "\tmov\t$1, %%rax\n");
    fprintf(file, "\tcmp\t%s, %%rax\n", regs64[r]);
    fprintf(file, "\tjne\tL%d\n", endlbl);

    regfree(r);

    DISCARD(gen_code(ast->forloop.body, file));
    DISCARD(gen_code(ast->forloop.update, file));

    fprintf(file, "\tjmp\tL%d\n", looplbl);
    fprintf(file, "L%d:\n", endlbl);
}

static int gen_strlit(struct ast *ast, FILE *file)
{
    int r = regalloc();
    int l = add_string(ast->strlit.str, file);

    fprintf(file, "\tleaq\tL%d(%%rip), %s\n", l, regs64[r]);
    return r;
}

static int gen_sizeof(struct ast *ast, FILE *file)
{
    int r = regalloc();
    fprintf(file, "\tmov\t$%lu, %s\n", asm_sizeof(ast->sizeofop.t), regs64[r]);
    return r;
}

/*static int gen_arracc(struct ast *ast, FILE *file)
{
    int r1 = gen_code(ast->arracc.arr, file);
    int r2 = gen_code(ast->arracc.off, file);

    int r3 = asm_addrof(sym_lookup(s_currscope, ast->unary.val->ident.name), file);
    fprintf(file, "\tadd\t%s, %s\n", regs64[r3], regs64[r1]);
    
    regfree(r3);
    regfree(r2);
    
    if (!ast->lvalue)
    {
        int r4 = regalloc();
        fprintf(file, "\tmov\t(%s), %s\n", regs64[r1], regs64[r4]);
        regfree(r1);
        return r4;
    }

    return r1;
}*/

// Generate code for an AST node
int gen_code(struct ast *ast, FILE *file)
{
    switch (ast->type)
    {
        case A_BINOP:  return gen_binop(ast, file);
        case A_UNARY:  return gen_unary(ast, file);
        case A_INTLIT: return gen_intlit(ast, file);
        case A_CALL:   return gen_call(ast, file);
        case A_IDENT:  return gen_ident(ast, file);
        case A_STRLIT: return gen_strlit(ast, file);
        case A_SIZEOF: return gen_sizeof(ast, file);
        //case A_ARRACC: return gen_arracc(ast, file);
        case A_FUNCDEF:
            gen_funcdef(ast, file);
            return NOREG;
        case A_ASM:
            gen_inlineasm(ast, file);
            return NOREG;
        case A_BLOCK:
            gen_block(ast, file);
            return NOREG;
        case A_RETURN:
            gen_return(ast, file);
            return NOREG;
        case A_IFELSE:
            gen_ifelse(ast, file);
            return NOREG;
        case A_WHILE:
            gen_while(ast, file);
            return NOREG;
        case A_FOR:
            gen_for(ast, file);
            return NOREG;
    }

    return NOREG;
}

void gen_ast(struct ast *ast, FILE *file)
{
    for (unsigned int i = 0; i < ast->block.symtab.cnt; i++)
    {
        struct sym *sym = &ast->block.symtab.syms[i];
        if (sym->attr & SYM_GLOBAL && sym->attr & SYM_VAR)
            fprintf(file, "\t.comm %s, %lu\n", sym->name, asm_sizeof(sym->type));
    }

    gen_code(ast, file);
}