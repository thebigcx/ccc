#include <gen.h>
#include <assert.h>

static const char *regs8[]  = { "%r8b", "%r9b", "%r10b", "%r11b" };
static const char *regs16[] = { "%r8w", "%r9w", "%r10w", "%r11w" };
static const char *regs32[] = { "%r8d", "%r9d", "%r10d", "%r11d" };
static const char *regs64[] = { "%r8",  "%r9",  "%r10",  "%r11" };

#define REGCNT (sizeof(regs64) / sizeof(regs64[0]))
#define NOREG (-1)

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
        fprintf(file, "\tmov %s, %s(%%rip)\n", regs64[r], ast->binop.lhs->ident.name);
        return r;
    }

    int r1 = gen_code(ast->binop.lhs, file);
    int r2 = gen_code(ast->binop.rhs, file);

    switch (ast->binop.op)
    {
        case OP_PLUS:        
            fprintf(file, "\tadd %s, %s\n", regs64[r1], regs64[r2]);
            break;
        case OP_MINUS:
            fprintf(file, "\tsub %s, %s\n", regs64[r1], regs64[r2]);
            break;
        case OP_MUL:
            fprintf(file, "\timul %s, %s\n", regs64[r1], regs64[r2]);
            break;
        case OP_DIV: break;

        case OP_EQUAL:
        {
            int r = regalloc();
            fprintf(file, "\tcmp %s, %s\n", regs64[r1], regs64[r2]);
            fprintf(file, "\tsete %%al\n");
            fprintf(file, "\tmovzx %%al, %s\n", regs64[r]);
            
            regfree(r1);
            regfree(r2);
            return r;
        }

    }

    regfree(r1);
    return r2;
}

static int gen_intlit(struct ast *ast, FILE *file)
{
    int r = regalloc();
    fprintf(file, "\tmov $%ld, %s\n", ast->intlit.ival, regs64[r]);
    return r;
}

static void gen_vardef(struct ast *ast, FILE *file)
{
    fprintf(file, "\t.comm %s, %d\n", ast->vardef.name, 4);
}

static int gen_ident(struct ast *ast, FILE *file)
{
    int r = regalloc();
    fprintf(file, "\tmov %s(%%rip), %s\n", ast->ident.name, regs64[r]);
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

static int gen_call(struct ast *ast, FILE *file)
{
    int r = regalloc();
    fprintf(file, "\tcall %s\n", ast->call.name);
    fprintf(file, "\tmov %%rax, %s\n", regs64[r]);
    return r;
}

static void gen_return(struct ast *ast, FILE *file)
{
    if (ast->ret.val)
    {
        int r = gen_code(ast->ret.val, file);
        fprintf(file, "\tmov %s, %%rax\n", regs64[r]);
    }
    
    fprintf(file, "\tret\n");
}

static void gen_ifelse(struct ast *ast, FILE *file)
{
    int r = gen_code(ast->ifelse.cond, file);

    int elselbl = -1;
    if (ast->ifelse.elseblock) elselbl = label();
    
    int endlbl = label();

    fprintf(file, "\tmov $1, %%rax\n");
    fprintf(file, "\tcmp %s, %%rax\n", regs64[r]);
    fprintf(file, "\tjne .L%d\n", elselbl != -1 ? elselbl : endlbl);

    gen_code(ast->ifelse.ifblock, file);
    
    if (elselbl != -1)
    {
        fprintf(file, ".L%d:\n", elselbl);
        gen_code(ast->ifelse.elseblock, file);
    }

    fprintf(file, ".L%d:\n", endlbl);
}

static void gen_while(struct ast *ast, FILE *file)
{
    int looplbl = label(), endlbl = label();

    fprintf(file, ".L%d:\n", looplbl);
    int r = gen_code(ast->ifelse.cond, file);
    
    fprintf(file, "\tmov $1, %%rax\n");
    fprintf(file, "\tcmp %s, %%rax\n", regs64[r]);
    fprintf(file, "\tjne .L%d\n", endlbl);

    gen_code(ast->whileloop.body, file);

    fprintf(file, "\tjmp .L%d\n", looplbl);

    fprintf(file, ".L%d:\n", endlbl);
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
        case A_CALL:
            return gen_call(ast, file);
        case A_RETURN:
            gen_return(ast, file);
            return NOREG;
        case A_IFELSE:
            gen_ifelse(ast, file);
            return NOREG;
        case A_WHILE:
            gen_while(ast, file);
            return NOREG;
    }

    return NOREG;
}