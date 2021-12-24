#include <lexer.h>
#include <stddef.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <util.h>
#include <stdio.h>

struct lexer
{
    int currline, currcol;
    const char *str;
    struct token **toks;
    size_t tokcnt;
    const char *currfile;
};

static struct lexer s_lexer;

struct keyword
{
    const char *str;
    int token;
};

static struct keyword keywords[] =
{
    { "int8",    T_INT8    },
    { "int16",   T_INT16   },
    { "int32",   T_INT32   },
    { "int64",   T_INT64   },
    { "uint8",   T_UINT8   },
    { "uint16",  T_UINT16  },
    { "uint32",  T_UINT32  },
    { "uint64",  T_UINT64  },
    { "float32", T_FLOAT32 },
    { "float64", T_FLOAT64 },
    { "bool",    T_UINT8   },
    { "return",  T_RETURN  },
    { "if",      T_IF      },
    { "else",    T_ELSE    },
    { "while",   T_WHILE   },
    { "for",     T_FOR     },
    { "var",     T_VAR     },
    { "fn",      T_FUNC    },
    { "sizeof",  T_SIZEOF  },
    { "goto",    T_GOTO    },
    { "label",   T_LABEL   },
    { "typedef", T_TYPEDEF },
    { "extern",  T_EXTERN  },
    { "public",  T_PUBLIC  },
    { "struct",  T_STRUCT  },
    { "union",   T_UNION   },
    { "enum",    T_ENUM    },
};

// Tests whether 'c' is a valid character in an identifier - letters, numbers, or '_'
static int isidentc(char c)
{
    return isalnum(c) || c == '_';
}

static int iskeyword(const char *str)
{
    for (size_t i = 0; i < ARRLEN(keywords); i++)
        if (!strcmp(keywords[i].str, str)) return 1;
    
    return 0;
}

static void push(struct token t)
{
    t.line = s_lexer.currline;
    t.col  = s_lexer.currcol;
    t.file = s_lexer.currfile;
    *s_lexer.toks = realloc(*s_lexer.toks, (s_lexer.tokcnt + 1) * sizeof(struct token));
    (*s_lexer.toks)[s_lexer.tokcnt++] = t;
}

// Push token without value e.g. +, -, {
static void pushnv(int t)
{
    push((struct token) { .type = t });
}

static void pushi(unsigned long i)
{
    push((struct token) { .type = T_INTLIT, .v.ival = i });
}

static void pushasm()
{
    while (*s_lexer.str++ != '{');
    while (*s_lexer.str++ != '\n');

    char asmbuf[1024]; // TODO: dynamic
    char *asmptr = asmbuf;

    while (*s_lexer.str != '}') *asmptr++ = *s_lexer.str++;
    *asmptr = 0;
    s_lexer.str++;

    push((struct token) { .type = T_ASM, .v.sval = strdup(asmbuf) });
}

static void push_keyword(const char *str)
{
    int type = 0; // Will not fail, this function is preceded by a check to iskeyword()

    for (size_t i = 0; i < ARRLEN(keywords); i++)
        if (!strcmp(keywords[i].str, str)) type = keywords[i].token;

    pushnv(type);
}

static void push_ident(const char *str)
{
    push((struct token) { .type = T_IDENT, .v.sval = strdup(str) });
}

static void push_strlit()
{
    char buf[128];
    char *bufptr = buf;
    s_lexer.str++;

    // TODO: escape characters
    while (*s_lexer.str != '"') *bufptr++ = *s_lexer.str++;
    *bufptr = 0;
    s_lexer.str++;

    push((struct token) { .type = T_STRLIT, .v.sval = strdup(buf) });
}

int tokenize(const char *str, struct token **toks)
{
    s_lexer.currline = 1;
    s_lexer.currcol  = 0;
    s_lexer.str      = str;
    s_lexer.toks     = toks;
    s_lexer.tokcnt   = 0;

    while (*s_lexer.str)
    {
        switch (*s_lexer.str)
        {
            case '#':
            {
                s_lexer.str += 2;
                unsigned int line = strtoull(s_lexer.str, (char**)&s_lexer.str, 10);
                
                s_lexer.str += 2;
            
                char *file = strndup(s_lexer.str, strchr(s_lexer.str, '"') - s_lexer.str);
                while (*s_lexer.str++ != '\n');

                s_lexer.currline = line;
                s_lexer.currfile = file;
                continue;
            }

            case '+':
            {
                switch (*(++s_lexer.str))
                {
                    case '+': pushnv(T_INC); s_lexer.str++; break;
                    case '=': pushnv(T_PLUSEQ); s_lexer.str++; break;
                    default: pushnv(T_PLUS);
                }
                continue;
            }
            
            case '-':
            {
                switch (*(++s_lexer.str))
                {
                    case '-': pushnv(T_DEC); s_lexer.str++; break;
                    case '=': pushnv(T_MINUSEQ); s_lexer.str++; break;
                    case '>': pushnv(T_ARROW); s_lexer.str++; break;
                    default:  pushnv(T_MINUS);
                }
                continue;
            }

            case '*':
                if (*(++s_lexer.str) == '=')
                {
                    pushnv(T_MULEQ);
                    s_lexer.str++;
                }
                else pushnv(T_STAR);
                continue;

            case ';': pushnv(T_SEMI);    s_lexer.str++; continue;
            case '(': pushnv(T_LPAREN);  s_lexer.str++; continue;
            case ')': pushnv(T_RPAREN);  s_lexer.str++; continue;
            case '[': pushnv(T_LBRACK);  s_lexer.str++; continue;
            case ']': pushnv(T_RBRACK);  s_lexer.str++; continue;
            case '{': pushnv(T_LBRACE);  s_lexer.str++; continue;
            case '}': pushnv(T_RBRACE);  s_lexer.str++; continue;
            case ',': pushnv(T_COMMA);   s_lexer.str++; continue;
            case ':': pushnv(T_COLON);   s_lexer.str++; continue;
            case '~': pushnv(T_BITNOT);  s_lexer.str++; continue;
            case '/': pushnv(T_SLASH);   s_lexer.str++; continue;
            case '?': pushnv(T_TERNARY); s_lexer.str++; continue;

            case '.':
            {
                if (*(++s_lexer.str) == '.')
                {
                    if (*(++s_lexer.str) == '.')
                    {
                        pushnv(T_ELLIPSIS);
                        s_lexer.str++;
                    }
                    // TODO: lexer error
                }
                else pushnv(T_DOT);
                continue;
            }


            case '!':
                if (*(++s_lexer.str) == '=')
                {
                    pushnv(T_NEQ);
                    s_lexer.str++;
                }
                else pushnv(T_NOT);
                continue;

            case '=':
                if (*(++s_lexer.str) == '=')
                {
                    pushnv(T_EQEQ);
                    s_lexer.str++;
                }
                else pushnv(T_EQ);
                continue;

            case '>':
                switch (*(++s_lexer.str))
                {
                    case '=': pushnv(T_GTE); s_lexer.str++; break;
                    case '>':
                        if (*(++s_lexer.str) == '=')
                        {
                            pushnv(T_SHREQ);
                            s_lexer.str++;
                        }
                        else pushnv(T_SHR);
                        break;

                    default: pushnv(T_GT);
                }
                continue;
            
            case '<':
                switch (*(++s_lexer.str))
                {
                    case '=': pushnv(T_LTE); s_lexer.str++; break;
                    case '<':
                        if (*(++s_lexer.str) == '=')
                        {
                            pushnv(T_SHLEQ);
                            s_lexer.str++;
                        }
                        else pushnv(T_SHL);
                        break;
                    
                    default: pushnv(T_LT); break;
                }
                continue;

            case '"':
                push_strlit();
                continue;

            case '\'':
                s_lexer.str++;
                pushi(*s_lexer.str);
                s_lexer.str += 2;
                continue;

            case '&':
                switch (*(++s_lexer.str))
                {
                    case '&': pushnv(T_LAND); s_lexer.str++; break;
                    case '=': pushnv(T_ANDEQ); s_lexer.str++; break;
                    default: pushnv(T_AMP); break;
                }
                continue;
            
            case '|':
                switch (*(++s_lexer.str))
                {
                    case '|': pushnv(T_LOR); s_lexer.str++; break;
                    case '=': pushnv(T_OREQ); s_lexer.str++; break;
                    default:  pushnv(T_BITOR); break;
                }
                continue;

            case '^':
                if (*(++s_lexer.str) == '=')
                {
                    pushnv(T_XOREQ);
                    s_lexer.str++;
                }
                else pushnv(T_BITXOR);
                continue;

            case '%':
                if (*(++s_lexer.str) == '=')
                {
                    pushnv(T_MODEQ);
                    s_lexer.str++;
                }
                else pushnv(T_MOD);
                continue;
        }

        if (*s_lexer.str == -1)
        {
            pushnv(T_EOF);
            s_lexer.str++;
        }
        if (isspace(*s_lexer.str))
        {
            switch (*s_lexer.str)
            {
                case ' ':  s_lexer.currcol++; break;
                case '\t': s_lexer.currcol += 4; break;
                case '\n': s_lexer.currline++; break;
            }
            s_lexer.str++;
        }
        else if (isdigit(*s_lexer.str))
            pushi(strtoull(s_lexer.str, (char**)&s_lexer.str, 10));
        else if (isalpha(*s_lexer.str) || *s_lexer.str == '_')
        {
            char ident[64];
            size_t j;
            for (j = 0; isidentc(*s_lexer.str) && j < 63; j++, s_lexer.str++) ident[j] = *s_lexer.str;
            ident[j] = 0;

            if (!strcmp(ident, "asm"))
                pushasm();
            else if (iskeyword(ident))
                push_keyword(ident);
            else
                push_ident(ident);
        }
        else
        {
            printf("Invalid token: '%c'\n", *s_lexer.str);
            abort();
        }
    }

    pushnv(T_END);
    return 0;
}
