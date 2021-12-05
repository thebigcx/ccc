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
    assert(r && r < (int)REGCNT);
    reglist[r] = 0;
}

// gen_* functions return the register

static int gen_binop(struct ast *ast, FILE *file)
{
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
        case OP_DIV:
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
    }

    return NOREG;
}