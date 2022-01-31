#include "parse.h"
#include "sym.h"
#include "lib.h"

#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>

static char *s_str = NULL;

static void parse_reg(struct codeop *op)
{
    op->type = OP_REG;

    s_str++;

    if (*s_str == 'e')      { op->type |= OP_SIZE32; s_str++; }
    else if (*s_str == 'r') { op->type |= OP_SIZE64; s_str++; }
   
    switch (*s_str)
    {
        case 'a': op->val = REG_AX; s_str++; break;
        case 'c': op->val = REG_CX; s_str++; break;
        case 's': op->val = *(++s_str) == 'p' ? REG_SP : REG_SI; s_str++; break;
        case 'd':
            if (*(++s_str) == 'i') { op->val = REG_DI; s_str++; }
            else op->val = REG_DX;
            break;
        case 'b':
            if (*(++s_str) == 'p') { op->val = REG_BP; s_str++; }
            else op->val = REG_BX;
            break;
        case 'i':
            op->val = REG_RIP;
            s_str += 2;
            break;

        default:
            op->val = strtol(s_str, (char**)&s_str, 10);
   
            op->type = OP_REG;
            switch (*s_str)
            {
                case 'b': op->type |= OP_SIZE8; s_str++; break;
                case 'w': op->type |= OP_SIZE16; s_str++; break;
                case 'd': op->type |= OP_SIZE32; s_str++; break;
                default:  op->type |= OP_SIZE64; break;
            }

            return;
    }
    
    if (*s_str == 'l') { op->type |= OP_SIZE8; s_str++; }
    else if (!(op->type & OP_SIZEM)) op->type |= OP_SIZE16;

    if (*s_str == 'x') s_str++;
}

static uint64_t parse_digit_op()
{
    return xstrtonum(s_str, &s_str);
}

static struct codeop parse_op()
{
    struct codeop op = { 0 };

    if (!strncmp(s_str, "u8 ", 3)) { op.type |= OP_SIZE8; s_str += 3; }
    else if (!strncmp(s_str, "u16 ", 4)) { op.type |= OP_SIZE16; s_str += 4; }
    else if (!strncmp(s_str, "u32 ", 4)) { op.type |= OP_SIZE32; s_str += 4; }
    else if (!strncmp(s_str, "u64 ", 4)) { op.type |= OP_SIZE64; s_str += 4; }

    switch (*s_str)
    {
        case '%':
            parse_reg(&op);
            break;

        case '$': // TODO: evaluate constant expressions
            if (!op.type) op.type |= OP_ALLSZ;
            op.type |= OP_IMM;

            if (isdigit(*(++s_str))) op.val = parse_digit_op();
            else
            {
                op.sym = strndup(s_str, strpbrk(s_str, ",\n") - s_str);
                s_str = strpbrk(s_str, ",\n");
            }

            break;

        default:
        {
            //if (!isdigit(*str) && *str != '-' && *str != '(') error(); // TODO

            op.type |= OP_MEM;
            op.sib.base = REG_NUL;
            op.sib.idx = REG_NUL;

            if (isdigit(*s_str) || *s_str == '-')
            {
                op.val = parse_digit_op();
            }
            else if (isalpha(*s_str) || *s_str == '_')
            {
                op.sym = strndup(s_str, strpbrk(s_str, ",\n(") - s_str);
                s_str = strpbrk(s_str, ",\n(");
            }
            else op.sib.flags |= SIB_NODISP;

            if (*s_str != '(') break;

            s_str++;

            struct codeop reg = { 0 };
            if (*s_str != ',')
            {
                parse_reg(&reg);
                op.sib.base = reg.val;
                if (ISREGSZ(reg.type, OP_SIZE32)) op.sib.flags |= SIB_32BIT;
                else if (ISREGSZ(reg.type, OP_SIZE16)) op.sib.flags |= SIB_16BIT;
            }

            if (*s_str++ == ')') break;

            if (*s_str != ',')
            {
                parse_reg(&reg);
                op.sib.idx = reg.val;
                if (ISREGSZ(reg.type, OP_SIZE32)) op.sib.flags |= SIB_32BIT;
                else if (ISREGSZ(reg.type, OP_SIZE16)) op.sib.flags |= SIB_16BIT;
            }
            s_str++;

            if (*s_str != ')') op.sib.scale = parse_digit_op();

            s_str++;
            break;
        }
    }

    return op;
}

struct code parse_code(const char *str)
{
    s_str = (char*)str;

    const char *mnem = s_str;

    struct code code = { 0 };
    code.mnem = strndup(mnem, strpbrk(mnem, " \n") - mnem);

    if (*strpbrk(mnem, " \n") == '\n') return code;

    s_str = strchr(mnem, ' ') + 1;
    code.op1 = parse_op();

    if (s_str[0] == '\n') return code;
    s_str += 2;

    code.op2 = parse_op();
  
    if (code.op1.type & OP_MEM && !(code.op1.type & OP_SIZEM))
    {
        code.op1.type &= ~OP_SIZEM;
        code.op1.type |= code.op2.type & OP_SIZEM;
    }

    if (code.op2.type & OP_MEM && !(code.op2.type & OP_SIZEM))
    {
        code.op2.type &= ~OP_SIZEM;
        code.op2.type |= code.op1.type & OP_SIZEM;
    }

    return code;
}
