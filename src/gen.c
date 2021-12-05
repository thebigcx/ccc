#include <gen.h>
#include <assert.h>

static const char *regs[] = { "%rax", "%rbx", "%rcx", "%rdx" };

#define REGCNT (sizeof(regs) / sizeof(regs[0]))
#define NOREG (-1)

static int reglist[REGCNT] = { 0 };

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
    return NOREG;
}

static void regfree(int r)
{
    assert(r >= 0 && r < (int)REGCNT);
    reglist[r] = 0;
}

// gen_* functions return the register

static int gen_binop(struct ast *ast, FILE *file)
{
    // TODO: A_ASSIGN instead
    if (ast->binop.op == OP_ASSIGN)
    {
        int r = gen_code(ast->binop.rhs, file);
        fprintf(file, "\tmov %s, %s(%%rip)\n", regs[r], ast->binop.lhs->ident.name);
        return r;
    }

    int r1 = gen_code(ast->binop.lhs, file);
    int r2 = gen_code(ast->binop.rhs, file);

    switch (ast->binop.op)
    {
        case OP_PLUS:        
            fprintf(file, "\tadd %s, %s\n", regs[r1], regs[r2]);
            break;
        case OP_MINUS:
            fprintf(file, "\tsub %s, %s\n", regs[r1], regs[r2]);
            break;
        case OP_MUL:
            fprintf(file, "\timul %s, %s\n", regs[r1], regs[r2]);
            break;
        case OP_DIV: break;
    }

    regfree(r1);
    return r2;
}

static int gen_intlit(struct ast *ast, FILE *file)
{
    int r = regalloc();
    fprintf(file, "\tmov $%ld, %s\n", ast->intlit.ival, regs[r]);
    return r;
}

static void gen_vardef(struct ast *ast, FILE *file)
{
    fprintf(file, "\t.comm %s, %d\n", ast->vardef.name, 4);
}

static int gen_ident(struct ast *ast, FILE *file)
{
    int r = regalloc();
    fprintf(file, "\tmov %s(%%rip), %s\n", ast->ident.name, regs[r]);
    return r;
}

static void gen_block(struct ast *ast, FILE *file)
{
    for (unsigned int i = 0; i < ast->block.cnt; i++)
        gen_code(ast->block.statements[i], file);
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

// Generate code for an AST node
int gen_code(struct ast *ast, FILE *file)
{
    switch (ast->type)
    {
        case A_BINOP:
            return gen_binop(ast, file);
        case A_INTLIT:
            return gen_intlit(ast, file);
        case A_VARDEF:
            gen_vardef(ast, file);
            return NOREG;
        case A_IDENT:
            return gen_ident(ast, file);
        case A_FUNCDEF:
            gen_funcdef(ast, file);
            return NOREG;
        case A_ASM:
            gen_inlineasm(ast, file);
            return NOREG;
        case A_BLOCK:
            gen_block(ast, file);
            return NOREG;
    }

    return NOREG;
}