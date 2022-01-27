#include "gen.h"
#include "sym.h"
#include "asm.h"
#include "ast.h"
#include "decl.h"

#include <assert.h>
#include <stdlib.h>

#define CMPEXPR(ast) (ast->type == A_BINOP && ((ast->binop.op > OP_LT && ast->binop.op < OP_NEQUAL) || ast->binop.op == OP_LAND || ast->binop.op == OP_LOR))

// Only for the .text section
#define CODE_INST 0
#define CODE_REG  1
#define CODE_LBL  2

struct code
{
    struct code *lhs, *rhs;
    int type;
    size_t size; // For determining registers and suffix

    union
    {
        int ival;
        char *str;
    };
};

enum REGS
{
    REG_8,
    REG_AL,
    REG_BL,
    REG_CL,
    REG_DL,
    REG_SIL,
    REG_DIL,
    REG_R8B,
    REG_R9B,
    REG_R10B,
    REG_R11B,
    REG_R12B,
    REG_R13B,
    REG_R14B,
    REG_R15B,

    REG_16,
    REG_AX,
    REG_BX,
    REG_CX,
    REG_DX,
    REG_SI,
    REG_DI,
    REG_R8W,
    REG_R9W,
    REG_R10W,
    REG_R11W,
    REG_R12W,
    REG_R13W,
    REG_R14W,
    REG_R15W,

    REG_32,
    REG_EAX,
    REG_EBX,
    REG_ECX,
    REG_EDX,
    REG_ESI,
    REG_EDI,
    REG_R8D,
    REG_R9D,
    REG_R10D,
    REG_R11D,
    REG_R12D,
    REG_R13D,
    REG_R14D,
    REG_R15D,

    REG_64,
    REG_RAX,
    REG_RBX,
    REG_RCX,
    REG_RDX,
    REG_RSI,
    REG_RDI,
    REG_R8,
    REG_R9,
    REG_R10,
    REG_R11,
    REG_R12,
    REG_R13,
    REG_R14,
    REG_R15,
};

const char *regstrs[] =
{
    [REG_AL] = "%al",
    [REG_BL] = "%bl",
    [REG_CL] = "%cl",
    [REG_DL] = "%dl",
    [REG_SIL] = "%sil",
    [REG_DIL] = "%dil",
    [REG_R8B] = "%r8b",
    [REG_R9B] = "%r9b",
    [REG_R10B] = "%r10b",
    [REG_R11B] = "%r11b",
    [REG_R12B] = "%r12b",
    [REG_R13B] = "%r13b",
    [REG_R14B] = "%r14b",
    [REG_R15B] = "%r15b",

    [REG_AX] = "%ax",
    [REG_BX] = "%bx",
    [REG_CX] = "%cx",
    [REG_DX] = "%dx",
    [REG_SI] = "%si",
    [REG_DI] = "%di",
    [REG_R8W] = "%r8w",
    [REG_R9W] = "%r9w",
    [REG_R10W] = "%r10w",
    [REG_R11W] = "%r11w",
    [REG_R12W] = "%r12w",
    [REG_R13W] = "%r13w",
    [REG_R14W] = "%r14w",
    [REG_R15W] = "%r15w",

    [REG_EAX] = "%eax",
    [REG_EBX] = "%ebx",
    [REG_ECX] = "%ecx",
    [REG_EDX] = "%edx",
    [REG_ESI] = "%esi",
    [REG_EDI] = "%edi",
    [REG_R8D] = "%r8d",
    [REG_R9D] = "%r9d",
    [REG_R10D] = "%r10d",
    [REG_R11D] = "%r11d",
    [REG_R12D] = "%r12d",
    [REG_R13D] = "%r13d",
    [REG_R14D] = "%r14d",
    [REG_R15D] = "%r15d",

    [REG_RAX] = "%rax",
    [REG_RBX] = "%rbx",
    [REG_RCX] = "%rcx",
    [REG_RDX] = "%rdx",
    [REG_RSI] = "%rsi",
    [REG_RDI] = "%rdi",
    [REG_R8] = "%r8",
    [REG_R9] = "%r9",
    [REG_R10] = "%r10",
    [REG_R11] = "%r11",
    [REG_R12] = "%r12",
    [REG_R13] = "%r13",
    [REG_R14] = "%r14",
    [REG_R15] = "%r15"
};

enum INSTS
{
    INST_MOV,
    INST_LEA,
    INST_ADD,
    INST_SUB,
    INST_IMUL,
    INST_IDIV,
    INST_PUSH,
    INST_POP,
    INST_MOVZBQ,
    INST_MOVZWQ,
    INST_MOVSLQ,
    INST_CQO,
    INST_SHL,
    INST_SHR,
    INST_OR,
    INST_AND,
    INST_NOT,
    INST_TEST,
    INST_CMP,
    INST_SETE,
    INST_SETNE,
    INST_SETG,
    INST_SETL,
    INST_SETGE,
    INST_SETLE,
    INST_JZ,
    INST_JNZ,
    INST_JG,
    INST_JL,
    INST_JGE,
    INST_JLE,
    INST_NEG,
    INST_INC,
    INST_DEC,
    INST_CALL,
    INST_RET,
    INST_LEAVE,
    INST_JMP
};

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

static int spillreg = 0;

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

    int r = (spillreg++ % REGCNT);
    fprintf(g_outf, "\tpush %s\n", regs64[r]);
    return r;
}

static void regfree(int r)
{
    if (spillreg > 0)
    {
        r = (--spillreg & REGCNT);
        fprintf(g_outf, "\tpop %s\n", regs64[r]);
    }
    else reglist[r] = 0;
}

static const char *setinsts[] =
{
    [OP_EQUAL]  = "setz",
    [OP_NEQUAL] = "setnz",
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

static int asm_addrof(struct sym *sym, int r)
{
    if (sym->attr & SYM_GLOBAL)
        //fprintf(g_outf, "\tleaq\t%s(%%rip), %s\n", sym->name, regs64[r]);
        fprintf(g_outf, "\tmov $%s, %s\n", sym->name, regs64[r]);
    else
        fprintf(g_outf, "\tlea -%lu(%%rbp), %s\n", sym->stackoff, regs64[r]);
    return r;
}

static const char **regs[9] =
{
    [1] = regs8,
    [2] = regs16,
    [4] = regs32,
    [8] = regs64
};

void asm_label(int lbl)
{
    fprintf(g_outf, "L%d:\n", lbl);
}

void asm_jump(int lbl)
{
    fprintf(g_outf, "\tjmp $L%d\n", lbl);
}

void asm_jumpn(const char *name)
{
    fprintf(g_outf, "\tjmp $%s\n", name);
}

void asm_section(const char *name)
{
    fprintf(g_outf, "\t.section %s\n", name);
}

void asm_string(const char *str)
{
    fprintf(g_outf, "\t.str \"%s\"\n", str);
}

void asm_symbol(struct sym *sym)
{
    if (sym->attr & SYM_PUBLIC)
        fprintf(g_outf, "\t.global %s\n", sym->name);
    fprintf(g_outf, "%s:\n", sym->name); 
}

void asm_dataprim(struct type t)
{
    if (t.ptr)
        fprintf(g_outf, "\t.long 0\n");
    else
    {
        switch (asm_sizeof(t))
        {
            case 1: fprintf(g_outf, "\t.byte  0\n"); break;
            case 2: fprintf(g_outf, "\t.short 0\n"); break;
            case 4: fprintf(g_outf, "\t.int   0\n"); break;
            case 8: fprintf(g_outf, "\t.long  0\n"); break;
        }
    }
}

static int asm_load(struct sym *sym, int r)
{
    if (sym->type.arrlen)
        return asm_addrof(sym, r);
    else
    {
        if (sym->attr & SYM_LOCAL)
            fprintf(g_outf, "\tmov -%lu(%%rbp), %s\n", sym->stackoff, regs[asm_sizeof(sym->type)][r]);
        else
            fprintf(g_outf, "\tmov %s, %s\n", sym->name, regs[asm_sizeof(sym->type)][r]);
        return r;
    }
}

static int asm_store(struct sym *sym, int r)
{
    if (sym->attr & SYM_LOCAL)
        fprintf(g_outf, "\tmov %s, -%lu(%%rbp)\n", regs[asm_sizeof(sym->type)][r], sym->stackoff);
    else
        fprintf(g_outf, "\tmov %s, %s(%%rip)\n", regs[asm_sizeof(sym->type)][r], sym->name);
    return r;
}

static int asm_storederef(struct ast *ast, int r1, int r2)
{
    fprintf(g_outf, "\tmov %s, (%s)\n", regs[asm_sizeof(ast->vtype)][r2], regs64[r1]);
    regfree(r1);
    return r2;
}

static int asm_loadderef(struct ast *ast, int r)
{
    if (ast->lvalue)
        return r;
    else
    {
        int r2 = regalloc();
        fprintf(g_outf, "\tmov (%s), %s\n", regs64[r], regs[asm_sizeof(ast->vtype)][r2]);
        regfree(r);
        return r2;
    }
}

static int asm_add(int r1, int r2)
{
    fprintf(g_outf, "\tadd %s, %s\n", regs64[r2], regs64[r1]);
    regfree(r2);
    return r1;
}

static int asm_sub(int r1, int r2)
{
    fprintf(g_outf, "\tsub %s, %s\n", regs64[r2], regs64[r1]);
    regfree(r2);
    return r1;
}

static int asm_mul(int r1, int r2)
{
    fprintf(g_outf, "\timul %s, %s\n", regs64[r2], regs64[r1]);
    regfree(r2);
    return r1;
}

static int asm_div(int r1, int r2)
{
    fprintf(g_outf, "\tcqo\n");
    fprintf(g_outf, "\tmov %s, %%rax\n", regs64[r1]);
    fprintf(g_outf, "\tidiv %s\n", regs64[r2]);
    fprintf(g_outf, "\tmov %%rax, %s\n", regs64[r1]);
    regfree(r2);
    return r1;
}

static int asm_mod(int r1, int r2)
{
    fprintf(g_outf, "\tcqo\n");
    fprintf(g_outf, "\tmov %s, %%rax\n", regs64[r1]);
    fprintf(g_outf, "\tidiv %s\n", regs64[r2]);
    fprintf(g_outf, "\tmov %%rdx, %s\n", regs64[r1]);
    regfree(r2);
    return r1;
}

static int asm_shl(int r1, int r2)
{
    fprintf(g_outf, "\tmov %s, %%cl\n", regs8[r2]);
    fprintf(g_outf, "\tshl %%cl, %s\n", regs64[r1]);
    regfree(r2);
    return r1;
}

static int asm_shr(int r1, int r2)
{
    fprintf(g_outf, "\tmov %s, %%cl\n", regs8[r2]);
    fprintf(g_outf, "\tshr %%cl, %s\n", regs64[r1]);
    regfree(r2);
    return r1;
}

static int asm_bitand(int r1, int r2)
{
    fprintf(g_outf, "\tand %s, %s\n", regs64[r2], regs64[r1]);
    regfree(r2);
    return r1;
}

static int asm_bitor(int r1, int r2)
{
    fprintf(g_outf, "\tor %s, %s\n", regs64[r2], regs64[r1]);
    regfree(r2);
    return r1;
}

static int asm_bitxor(int r1, int r2)
{
    fprintf(g_outf, "\txor %s, %s\n", regs64[r2], regs64[r1]);
    regfree(r2);
    return r1;
}

static int asm_cmpandset(int r1, int r2, int op)
{
    int r = regalloc();
    fprintf(g_outf, "\tcmp %s, %s\n", regs64[r2], regs64[r1]);
    fprintf(g_outf, "\t%s %%al\n", setinsts[op]);
    fprintf(g_outf, "\tmovzx %%al, %s\n", regs64[r]);
    
    regfree(r1);
    regfree(r2);
    return r;
}

// Compare two registers and jmp past code block
static int asm_cmpandjmp(int r1, int r2, int op, int lbl)
{
    fprintf(g_outf, "\tcmp %s, %s\n", regs64[r2], regs64[r1]);
    fprintf(g_outf, "\t%s $L%d\n", jmpinsts[op], lbl);
    regfree(r1);
    regfree(r2);
    return NOREG;
}

void asm_funcpre(size_t st)
{
    if (!st) return;

    fprintf(g_outf, "\tpush %%rbp\n");
    fprintf(g_outf, "\tmov %%rsp, %%rbp\n");
    fprintf(g_outf, "\tsub $%lu, %%rsp\n", st);
}

void asm_funcpost(size_t st)
{
    if (st) fprintf(g_outf, "\tleave\n");
    fprintf(g_outf, "\tret\n");
}

// Assignment binary expressions e.g. x = 10, x += 2, *x /= 2, etc
static int gen_assign(int r1, int r2, struct ast *ast)
{
    switch (ast->binop.op)
    {
        case OP_PLUSEQ:   r2 = asm_add(r1, r2); break;
        case OP_MULEQ:    r2 = asm_mul(r1, r2); break;
        case OP_DIVEQ:    r2 = asm_div(r1, r2); break;
        case OP_MINUSEQ:  r2 = asm_sub(r1, r2); break;
        case OP_BITANDEQ: r2 = asm_bitand(r1, r2); break;
        case OP_BITOREQ:  r2 = asm_bitor(r1, r2); break;
        case OP_BITXOREQ: r2 = asm_bitxor(r1, r2); break;
        case OP_SHLEQ:    r2 = asm_shl(r1, r2); break;
        case OP_SHREQ:    r2 = asm_shr(r1, r2); break;
        case OP_MODEQ:    r2 = asm_mod(r1, r2); break;
    }
    
    if (ast->binop.lhs->type == A_UNARY && ast->binop.lhs->unary.op == OP_DEREF)
        return asm_storederef(ast, r1, r2);
    else
    {
        struct sym *sym = sym_lookup(s_currscope, ast->binop.lhs->ident.name);
        return asm_store(sym, r2);
    }
}

static int gen_lazyeval(struct ast *ast)
{
    int end = label();

    int r1 = gen_code(ast->binop.lhs);

    asm_testandjmp(r1, end, ast->binop.op == OP_LAND);

    if (!CMPEXPR(ast->binop.lhs))
    {
        fprintf(g_outf, "\tsetne %s\n", regs8[r1]);
        fprintf(g_outf, "\tmovzbq %s, %s\n", regs8[r1], regs64[r1]);
    }

    int r2 = gen_code(ast->binop.rhs);
    if (!CMPEXPR(ast->binop.rhs))
    {
        fprintf(g_outf, "\ttest %s, %s\n", regs64[r2], regs64[r2]);
        fprintf(g_outf, "\tsetne %s\n", regs8[r2]);
        fprintf(g_outf, "\tmovzbq %s, %s\n", regs8[r2], regs64[r2]);
    }

    regfree(r1);
    asm_label(end);
    return r2;
}

static int gen_lazyevaljmp(int lbl, struct ast *ast)
{
    int r1 = gen_code(ast->binop.lhs);

    asm_testandjmp(r1, lbl, ast->binop.op == OP_LAND);
    regfree(r1);

    int r2 = gen_code(ast->binop.rhs);
    asm_testandjmp(r2, lbl, 1);
    regfree(r2);
    return NOREG;
}

static int gen_binop(struct ast *ast)
{
    if (ast->binop.op == OP_LAND || ast->binop.op == OP_LOR)
        return gen_lazyeval(ast);

    int r1 = gen_code(ast->binop.lhs);
    int r2 = gen_code(ast->binop.rhs);

    switch (ast->binop.op)
    {
        case OP_PLUS:   return asm_add(r1, r2); 
        case OP_MUL:    return asm_mul(r1, r2);
        case OP_DIV:    return asm_div(r1, r2);      
        case OP_MINUS:  return asm_sub(r1, r2);
        case OP_SHL:    return asm_shl(r1, r2);
        case OP_SHR:    return asm_shr(r1, r2);
        case OP_MOD:    return asm_mod(r1, r2);
        case OP_EQUAL:
        case OP_NEQUAL:
        case OP_GT:
        case OP_LT:
        case OP_GTE:
        case OP_LTE:    return asm_cmpandset(r1, r2, ast->binop.op);
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
        case OP_MODEQ:  return gen_assign(r1, r2, ast);
        case OP_BITAND: return asm_bitand(r1, r2);
        case OP_BITOR:  return asm_bitor(r1, r2);
        case OP_BITXOR: return asm_bitxor(r1, r2);
    }

    return NOREG;
}

int asm_lognot(int r)
{
    fprintf(g_outf, "\ttest %s, %s\n", regs64[r], regs64[r]);
    fprintf(g_outf, "\tsete %s\n", regs8[r]);
    fprintf(g_outf, "\tmovzbq %s, %s\n", regs8[r], regs64[r]);
    return r;
}

int asm_bitnot(int r)
{
    fprintf(g_outf, "\tnot %s\n", regs64[r]);
    return r;
}

int asm_unaryminus(int r)
{
    fprintf(g_outf, "\tneg %s\n", regs64[r]);
    return r;
}

void asm_testandjmp(int r, int lbl, int zf)
{
    fprintf(g_outf, "\ttest %s, %s\n", regs64[r], regs64[r]);
    fprintf(g_outf, "\t%s $L%d\n", zf ? "jz" : "jnz", lbl);
}

int asm_loadint(unsigned long i)
{
    int r = regalloc();
    fprintf(g_outf, "\tmov $%ld, %s\n", i, regs64[r]);
    return r;
}

void asm_move(int r1, int r2, size_t s1, size_t s2)
{
    if (s1 >= s2)
        fprintf(g_outf, "\tmov %s, %s\n", regs[s2][r1], regs[s2][r2]);
}

static int gen_addrof(struct ast *ast)
{
    int r = regalloc();
    return asm_addrof(sym_lookup(s_currscope, ast->unary.val->ident.name), r);
}

static int gen_unary(struct ast *ast)
{
    if (ast->unary.op == OP_ADDROF)
        return gen_addrof(ast);

    int r = gen_code(ast->unary.val);

    switch (ast->unary.op)
    {
        case OP_DEREF:  return asm_loadderef(ast, r); break;
        case OP_LOGNOT: return asm_lognot(r); break;
        case OP_BITNOT: return asm_bitnot(r); break;
        case OP_MINUS:  return asm_unaryminus(r); break;
    }

    return NOREG;
}

static int gen_ident(struct ast *ast)
{
    if (ast->lvalue) return NOREG;

    int r = regalloc();
    struct sym *sym = sym_lookup(s_currscope, ast->ident.name);
    return asm_load(sym, r);
}

static int gen_block(struct ast *ast)
{
    s_currscope = &ast->block.symtab;

    for (unsigned int i = 0; i < ast->block.cnt; i++)
        DISCARD(gen_code(ast->block.statements[i]));

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

static int gen_funcdef(struct ast *ast)
{
    ast->funcdef.endlbl = label();

    struct sym *sym = sym_lookup(s_currscope, ast->funcdef.name);
    asm_symbol(sym_lookup(s_currscope, ast->funcdef.name));

    size_t st = ast->funcdef.block->block.symtab.curr_stackoff;

    asm_funcpre(st);

    for (unsigned int i = 0; i < sym->type.func.paramcnt; i++)
    {
        struct sym *sym = sym_lookup(&ast->funcdef.block->block.symtab, ast->funcdef.params[i]);
        size_t size = asm_sizeof(sym->type);
        fprintf(g_outf, "\tmov %s, -%lu(%%rbp)\n", paramregs[size][i], sym->stackoff);
    }

    gen_block(ast->funcdef.block);
    asm_label(ast->funcdef.endlbl);
    asm_funcpost(st);

    return NOREG;
}

static int gen_inlineasm(struct ast *ast)
{
    fprintf(g_outf, "%s", ast->inasm.code);
    return NOREG;
}

static int gen_call(struct ast *ast)
{
    for (unsigned int i = 0; i < ast->call.paramcnt; i++)
    {
        int par = gen_code(ast->call.params[i]);
        size_t s = asm_sizeof(ast->call.params[i]->vtype);
        fprintf(g_outf, "\tmov %s, %s\n", regs[s][par], paramregs[s][i]);
        regfree(par);
    }

    const char *fn;
    if (ast->call.ast->type == A_UNARY && ast->call.ast->unary.op == OP_ADDROF
        && ast->call.ast->unary.val->type == A_IDENT)
        fn = ast->call.ast->unary.val->ident.name;
    else
    {
        int r = gen_code(ast->call.ast);
        fn = regs64[r];
        regfree(r); // A bit dodgy, makes for better looking code
    }

    for (int i = 0; i < (int)REGCNT; i++)
    {
        if (reglist[i])
            fprintf(g_outf, " push\t%s\n", regs64[i]);
    }

    if (ast->call.ast->vtype.func.variadic)
        fprintf(g_outf, "\txor %%rax, %%rax\n");
    fprintf(g_outf, "\tcall $%s\n", fn);

    for (int i = REGCNT - 1; i >= 0; i--)
    {
        if (reglist[i])
            fprintf(g_outf, "\tpop %s\n", regs64[i]);
    }

    if (ast->vtype.name == TYPE_VOID && !ast->vtype.ptr)
        return NOREG;

    int r = regalloc();
    fprintf(g_outf, "\tmov %%rax, %s\n", regs64[r]);
    return r;
}

void asm_retval(int r, size_t s)
{
    switch (s)
    {
        case 1: fprintf(g_outf, "\tmovzbl %s, %%eax\n", regs8[r]); break;
        case 2: fprintf(g_outf, "\tmovzwl %s, %%eax\n", regs16[r]); break;
        case 4: fprintf(g_outf, "\tmov %s, %%eax\n", regs32[r]); break;
        case 8: fprintf(g_outf, "\tmov %s, %%rax\n", regs64[r]); break;
    }
}

static int gen_return(struct ast *ast)
{
    if (ast->ret.val)
    {
        int r = gen_code(ast->ret.val);
        asm_retval(r, asm_sizeof(ast->ret.val->vtype));
        regfree(r);
    }

    asm_jump(ast->ret.func->funcdef.endlbl);
    return NOREG;
}

static int gen_ifelse(struct ast *ast)
{
    int elselbl = -1;
    if (ast->ifelse.elseblock) elselbl = label();
    
    int endlbl = label();

    struct ast *cond = ast->ifelse.cond;
    if (CMPEXPR(cond))
    {
        if (cond->binop.op == OP_LAND || cond->binop.op == OP_LOR)
            gen_lazyevaljmp(elselbl != -1 ? elselbl : endlbl, cond);
        else
        {
            int r1 = gen_code(cond->binop.lhs);
            int r2 = gen_code(cond->binop.rhs);
            asm_cmpandjmp(r1, r2, cond->binop.op, elselbl != -1 ? elselbl : endlbl);
        }
    }
    else
    {
        int r = gen_code(cond);
        asm_testandjmp(r, elselbl != -1 ? elselbl : endlbl, 1);
        regfree(r);
    }

    DISCARD(gen_code(ast->ifelse.ifblock));
    
    if (elselbl != -1)
    {
        asm_jump(endlbl);
        asm_label(elselbl);
        DISCARD(gen_code(ast->ifelse.elseblock));
    }

    asm_label(endlbl);
    return NOREG;
}

static int gen_while(struct ast *ast)
{
    int looplbl = label(), endlbl = label();

    asm_label(looplbl);
    int r = gen_code(ast->ifelse.cond);
   
    asm_testandjmp(r, endlbl, 1);

    regfree(r);

    DISCARD(gen_code(ast->whileloop.body));

    asm_jump(looplbl);
    asm_label(endlbl);
    return NOREG;
}

// TODO: refactor block/symtab stuff, because the code that follows is very ugly and hacky

static int gen_for(struct ast *ast)
{
    int looplbl = label(), endlbl = label();

    s_currscope = &ast->forloop.body->block.symtab;
    DISCARD(gen_code(ast->forloop.init));

    asm_label(looplbl);
    int r = gen_code(ast->forloop.cond);

    asm_testandjmp(r, endlbl, 1);

    regfree(r);

    DISCARD(gen_code(ast->forloop.body));
    // TODO: this is hacky
    s_currscope = &ast->forloop.body->block.symtab;
    DISCARD(gen_code(ast->forloop.update));

    s_currscope = ast->forloop.body->block.symtab.parent;
    
    asm_jump(looplbl);
    asm_label(endlbl);
    return NOREG;
}

static int gen_strlit(struct ast *ast)
{
    int r = regalloc();
    fprintf(g_outf, "\tmov $L%d, %s\n", s_globlscope->block.strs[ast->strlit.idx].lbl, regs64[r]);
    //fprintf(g_outf, "\tleaq\tL%d(%%rip), %s\n", s_globlscope->block.strs[ast->strlit.idx].lbl, regs64[r]);
    return r;
}

static int gen_sizeof(struct ast *ast)
{
    int r = regalloc();
    fprintf(g_outf, "\tmov $%lu, %s\n", asm_sizeof(ast->sizeofop.t), regs64[r]);
    return r;
}

void asm_labeln(const char *name)
{
    fprintf(g_outf, "%s:\n", name);
}

// Pre-increment or decrement
int gen_pre(struct ast *ast)
{
    const char *inst = ast->type == A_PREINC ? "inc" : "dec";

    int r1 = gen_code(ast->incdec.val);
    if (ast->incdec.val->type == A_UNARY && ast->incdec.val->unary.op == OP_DEREF)
    {
        int r2 = gen_code(ast->incdec.val->unary.val);
        
        fprintf(g_outf, "\t%s %s\n", inst, regs64[r1]);
        return asm_storederef(ast, r2, r1);
    }
    else
    {
        fprintf(g_outf, "\t%s %s\n", inst, regs64[r1]);
        struct sym *sym = sym_lookup(s_currscope, ast->incdec.val->ident.name);
        return asm_store(sym, r1);
    }
}

int gen_post(struct ast *ast)
{
    const char *inst = ast->type == A_POSTINC ? "inc" : "dec";

    int r1 = gen_code(ast->incdec.val);
    if (ast->incdec.val->type == A_UNARY && ast->incdec.val->unary.op == OP_DEREF)
    {
        int r2 = gen_code(ast->incdec.val->unary.val);
        int r3 = regalloc();

        asm_move(r1, r2, 8, 8);
        fprintf(g_outf, "\t%s %s\n", inst, regs64[r3]);

        int r = asm_storederef(ast, r2, r3);
        regfree(r1);
        regfree(r2);
        return r;
    }
    else
    {
        int r2 = regalloc();

        asm_move(r1, r2, 8, 8);
        fprintf(g_outf, "\t%s %s\n", inst, regs64[r2]);
        
        struct sym *sym = sym_lookup(s_currscope, ast->incdec.val->ident.name);
        regfree(asm_store(sym, r2));
        return r1;
    }
}

int gen_scale(struct ast *ast)
{
    int r = gen_code(ast->scale.val);
    fprintf(g_outf, "\timul $%u, %s\n", ast->scale.num, regs64[r]);
    return r;
}

int gen_ternary(struct ast *ast)
{
    int l1 = label(), l2 = label();
    int r1 = gen_code(ast->ternary.cond);
    int r2 = regalloc();

    asm_testandjmp(r1, l1, 1);
    regfree(r1);

    int r3 = gen_code(ast->ternary.lhs);
    asm_move(r3, r2, 8, 8);
    regfree(r3);

    asm_jump(l2);
    asm_label(l1);
    
    r3 = gen_code(ast->ternary.rhs);
    asm_move(r3, r2, 8, 8);
    regfree(r3);

    asm_label(l2);
    return r2;
}

// Generate code for an AST node
int gen_code(struct ast *ast)
{
    switch (ast->type)
    {
        case A_BINOP:   return gen_binop(ast);
        case A_UNARY:   return gen_unary(ast);
        case A_INTLIT:  return asm_loadint(ast->intlit.ival);
        case A_CALL:    return gen_call(ast);
        case A_IDENT:   return gen_ident(ast);
        case A_STRLIT:  return gen_strlit(ast);
        case A_SIZEOF:  return gen_sizeof(ast);
        case A_CAST:    return gen_code(ast->cast.val);
        case A_PREINC:
        case A_PREDEC:  return gen_pre(ast);
        case A_POSTINC:
        case A_POSTDEC: return gen_post(ast);
        case A_SCALE:   return gen_scale(ast);
        case A_FUNCDEF: return gen_funcdef(ast);
        case A_ASM:     return gen_inlineasm(ast);
        case A_BLOCK:   return gen_block(ast);
        case A_RETURN:  return gen_return(ast);
        case A_IFELSE:  return gen_ifelse(ast);
        case A_WHILE:   return gen_while(ast);
        case A_FOR:     return gen_for(ast);
        case A_LABEL:   asm_labeln(ast->label.name); return NOREG;
        case A_GOTO:    asm_jumpn(ast->gotolbl.label); return NOREG;
        case A_TERNARY: return gen_ternary(ast);
    }

    return NOREG;
}

void gen_datavar(struct type t)
{
    if (t.arrlen)
    {
        struct type base = t;
        base.arrlen = 0;

        for (int i = 0; i < t.arrlen; i++)
            gen_datavar(base);
    }
    else if (t.name == TYPE_STRUCT)
    {
        for (unsigned int i = 0; i < t.struc.memcnt; i++)
            gen_datavar(t.struc.members[i].type);
    }
    else asm_dataprim(t);
}

void gen_ast()
{
    s_globlscope = g_ast;

    asm_section(".rodata");

    for (unsigned int i = 0; i < g_ast->block.strcnt; i++)
    {
        asm_label(g_ast->block.strs[i].lbl = label());
        asm_string(g_ast->block.strs[i].val);
    }

    asm_section(".data");

    for (unsigned int i = 0; i < g_ast->block.symtab.cnt; i++)
    {
        struct sym *sym = &g_ast->block.symtab.syms[i];
        if (sym->attr & SYM_GLOBAL && !(sym->type.name == TYPE_FUNC && !sym->type.ptr))
        {
            asm_symbol(sym);
            gen_datavar(sym->type); 
        }
    }

    asm_section(".text");
    gen_code(g_ast);
}
