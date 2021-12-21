#include <gen.h>
#include <sym.h>
#include <asm.h>

#include <assert.h>
#include <stdlib.h>

static const char *regs8[]  = { "%r8b", "%r9b", "%r10b", "%r11b" };
static const char *regs16[] = { "%r8w", "%r9w", "%r10w", "%r11w" };
static const char *regs32[] = { "%r8d", "%r9d", "%r10d", "%r11d" };
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

static void asm_addrof(struct sym *sym, int r, FILE *file)
{
    if (sym->attr & SYM_GLOBAL)
        fprintf(file, "\tleaq\t%s(%%rip), %s\n", sym->name, regs64[r]);
    else
        fprintf(file, "\tleaq\t-%lu(%%rbp), %s\n", sym->stackoff, regs64[r]);
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

static void gen_load(struct sym *sym, int r, FILE *file)
{
    if (sym->type.arrlen)
        asm_addrof(sym, r, file);
    else
    {
        if (sym->attr & SYM_LOCAL)
            fprintf(file, "\t%s\t-%lu(%%rbp), %s\n", movzs[asm_sizeof(sym->type)], sym->stackoff, regs64[r]);
        else
            fprintf(file, "\t%s\t%s(%%rip), %s\n", movzs[asm_sizeof(sym->type)], sym->name, regs64[r]);
    }
}

static void gen_store(struct sym *sym, int r, FILE* file)
{
    if (sym->attr & SYM_LOCAL)
        fprintf(file, "\t%s\t%s, -%lu(%%rbp)\n", movs[asm_sizeof(sym->type)], regs[asm_sizeof(sym->type)][r], sym->stackoff);
    else
        fprintf(file, "\t%s\t%s, %s(%%rip)\n", movs[asm_sizeof(sym->type)], regs[asm_sizeof(sym->type)][r], sym->name);
}

static void gen_storederef(struct ast *ast, int r1, int r2, FILE *file)
{
    fprintf(file, "\t%s\t%s, (%s)\n", movs[asm_sizeof(ast->vtype)], regs[asm_sizeof(ast->vtype)][r1], regs64[r2]);
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
            fprintf(file, "\tsub\t%s, %s\n", regs64[r2], regs64[r1]);
            regfree(r2);
            return r1;
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
            fprintf(file, "\tcmp\t%s, %s\n", regs64[r2], regs64[r1]);
            fprintf(file, "\t%s\t%%al\n", set_instructions[ast->binop.op]);
            fprintf(file, "\tmovzx\t%%al, %s\n", regs64[r]);
            
            regfree(r1);
            regfree(r2);
            return r;
        }
        case OP_ASSIGN:
        {
            if (ast->binop.lhs->type == A_UNARY && ast->binop.lhs->unary.op == OP_DEREF)
                gen_storederef(ast, r2, r1, file);
            else
            {
                struct sym *sym = sym_lookup(s_currscope, ast->binop.lhs->ident.name);
                gen_store(sym, r2, file);
                return r2; // r1 is NOREG
            }
            
            break;
        }
        case OP_LAND:
        {
            fprintf(file, "\tand\t%s, %s\n", regs64[r1], regs64[r2]);
            fprintf(file, "\tand\t$1, %s\n", regs64[r2]);
            break;
        }
        case OP_LOR:
        {
            fprintf(file, "\tor\t%s, %s\n", regs64[r1], regs64[r2]);
            fprintf(file, "\tand\t$1, %s\n", regs64[r2]);
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
            int r = regalloc();
            asm_addrof(sym_lookup(s_currscope, ast->unary.val->ident.name), r, file);
            return r;
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
        case OP_NOT:
        {
            int r = gen_code(ast->unary.val, file);
            fprintf(file, "\tnot\t%s\n", regs64[r]);
            fprintf(file, "\tand\t$1, %s\n", regs64[r]);
            return r;
        }
        case OP_MINUS:
        {
            int r = gen_code(ast->unary.val, file);
            fprintf(file, "\tneg\t%s\n", regs64[r]);
            return r;
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
    if (ast->lvalue) return NOREG;

    int r = regalloc();

    struct sym *sym = sym_lookup(s_currscope, ast->ident.name);
    gen_load(sym, r, file);
    
    return r;
}

static void gen_block(struct ast *ast, FILE *file)
{
    s_currscope = &ast->block.symtab;

    for (unsigned int i = 0; i < ast->block.cnt; i++)
        DISCARD(gen_code(ast->block.statements[i], file));

    s_currscope = s_currscope->parent;
}

static void gen_funcdef(struct ast *ast, FILE *file)
{
    ast->funcdef.endlbl = label();

    if (sym_lookup(s_currscope, ast->funcdef.name)->attr & SYM_PUBLIC)
        fprintf(file, "\t.global %s\n", ast->funcdef.name);
    
    fprintf(file, "%s:\n", ast->funcdef.name);

    if (ast->funcdef.block->block.symtab.curr_stackoff)
    {
        fprintf(file, "\tpush\t%%rbp\n");
        fprintf(file, "\tmov\t%%rsp, %%rbp\n");
        fprintf(file, "\tsub\t$%lu, %%rsp\n", ast->funcdef.block->block.symtab.curr_stackoff);
    }

    gen_block(ast->funcdef.block, file);
    fprintf(file, ".L%d:\n", ast->funcdef.endlbl);

    if (ast->funcdef.block->block.symtab.curr_stackoff)
        fprintf(file, "\tleave\n");
    
    fprintf(file, "\tret\n\n");
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

    int r = gen_code(ast->call.ast, file);
    fprintf(file, "\tcall\t*%s\n", regs64[r]);
    regfree(r);

    r = regalloc();
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

    fprintf(file, "\tjmp\t.L%d\n", ast->ret.func->funcdef.endlbl);
}

static void gen_ifelse(struct ast *ast, FILE *file)
{
    int r = gen_code(ast->ifelse.cond, file);

    int elselbl = -1;
    if (ast->ifelse.elseblock) elselbl = label();
    
    int endlbl = label();

    fprintf(file, "\tmov\t$1, %%rax\n");
    fprintf(file, "\tcmp\t%%rax, %s\n", regs64[r]);
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

void gen_label(struct ast *ast, FILE *file)
{
    fprintf(file, "%s:\n", ast->label.name);
}

void gen_goto(struct ast *ast, FILE *file)
{
    fprintf(file, "\tjmp\t%s\n", ast->gotolbl.label);
}

int gen_cast(struct ast *ast, FILE *file)
{
    int r1 = gen_code(ast->cast.val, file);
    return r1;
    //int r2 = regalloc();

    // TODO: movz
    //return r2;

    //regfree(r1);
    //return r2;
}

int gen_preinc(struct ast *ast, FILE *file)
{
    (void)ast; (void)file;
    return NOREG;
}

int gen_postinc(struct ast *ast, FILE *file)
{
    (void)ast; (void)file;
    return NOREG;
}

int gen_predec(struct ast *ast, FILE *file)
{
    (void)ast; (void)file;
    return NOREG;
}

int gen_postdec(struct ast *ast, FILE *file)
{
    (void)ast; (void)file;
    return NOREG;
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
        case A_PREINC:  return gen_preinc(ast, file);
        case A_POSTINC: return gen_postinc(ast, file);
        case A_PREDEC:  return gen_predec(ast, file);
        case A_POSTDEC: return gen_postdec(ast, file);
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
        case A_LABEL:
            gen_label(ast, file);
            return NOREG;
        case A_GOTO:
            gen_goto(ast, file);
            return NOREG;
    }

    return NOREG;
}

void gen_ast(struct ast *ast, FILE *file)
{
    for (unsigned int i = 0; i < ast->block.symtab.cnt; i++)
    {
        struct sym *sym = &ast->block.symtab.syms[i];
        if (sym->attr & SYM_GLOBAL && !(sym->type.name == TYPE_FUNC && !sym->type.ptr))
            fprintf(file, "\t.comm %s, %lu\n", sym->name, asm_sizeof(sym->type));
    }

    gen_code(ast, file);
}