#include <lexer.h>
#include <defs.h>

#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#define ARRLEN(arr) (sizeof(arr) / sizeof(arr[0]))

static FILE *s_file = NULL;
static struct token **s_toks = NULL;
static size_t s_tokcnt = 0;

const char *insts[] =
{
    [INST_ADD] = "add",
    [INST_MOV] = "mov"
};

const char *regstrs[] =
{
    [REG_AH] = "ah",
    [REG_BH] = "bh",
    [REG_CH] = "ch",
    [REG_DH] = "dh",

    [REG_AL] = "al",
    [REG_BL] = "bl",
    [REG_CL] = "cl",
    [REG_DL] = "dl",
    [REG_SPL] = "spl",
    [REG_BPL] = "bpl",
    [REG_SIL] = "sil",
    [REG_DIL] = "dil",

    [REG_AX] = "ax",
    [REG_BX] = "bx",
    [REG_CX] = "cx",
    [REG_DX] = "dx",
    [REG_SP] = "sp",
    [REG_BP] = "bp",
    [REG_SI] = "si",
    [REG_DI] = "di",

    [REG_EAX] = "eax",
    [REG_EBX] = "ebx",
    [REG_ECX] = "ecx",
    [REG_EDX] = "edx",
    [REG_ESP] = "esp",
    [REG_EBP] = "ebp",
    [REG_ESI] = "esi",
    [REG_EDI] = "edi",

    [REG_RAX] = "rax",
    [REG_RBX] = "rbx",
    [REG_RCX] = "rcx",
    [REG_RDX] = "rdx",
    [REG_RSP] = "rsp",
    [REG_RBP] = "rbp",
    [REG_RSI] = "rsi",
    [REG_RDI] = "rdi"
};

int insttype(const char *sval)
{
    for (unsigned int i = 0; i < ARRLEN(insts); i++)
        if (!strcmp(insts[i], sval)) return i;
    return -1;
}

int regtype(const char *sval)
{
    for (unsigned int i = 0; i < ARRLEN(regstrs); i++)
        if (!strcmp(regstrs[i], sval)) return i;
    return -1;
}

void pushtok(int type, char *sval, unsigned long ival)
{
    *s_toks = realloc(*s_toks, (s_tokcnt + 1) * sizeof(struct token));
    (*s_toks)[s_tokcnt++] = (struct token) { .type = type, .sval = sval, .ival = ival };
}

int lexfile(FILE *file, struct token **toks)
{
    s_file = file;
    s_toks = toks;

    char c;
    while ((c = fgetc(file)) != EOF)
    {
        if (isspace(c)) continue;
        switch (c)
        {
            case ':': pushtok(T_COLON, NULL, 0); continue;
            case ',': pushtok(T_COMMA, NULL, 0); continue;
        }

        char str[32];
        int strl = 0;
        if (isalnum(c) || c == '_')
        {
            int dig = isdigit(c);

            do str[strl++] = c;
            while ((c = fgetc(file)) != EOF && isalnum(c));
            
            str[strl] = 0;
            ungetc(c, file);

            if (dig)
            {
                unsigned long v = strtoull(str, NULL, 10);
                pushtok(T_IMM, NULL, v);
            }
            else
            {
                int t;
                if ((t = insttype(str)) != -1) pushtok(T_INST, NULL, t);
                else if ((t = regtype(str)) != -1) pushtok(T_REG, NULL, t);
                else pushtok(T_LBL, strdup(str), 0);
            }
        }
    }

    pushtok(T_EOF, NULL, 0);
    return 0;
}
