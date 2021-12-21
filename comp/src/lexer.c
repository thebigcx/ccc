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
    { "enum",    T_ENUM    }
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

static void push(struct token t, struct token **toks, size_t *len)
{
    t.line = s_lexer.currline; t.col = s_lexer.currcol;
    *toks = realloc(*toks, (*len + 1) * sizeof(struct token));
    (*toks)[(*len)++] = t;
}

// Push token without value e.g. +, -, {
static void pushnv(int t, struct token **toks, size_t *len)
{
    push((struct token) { .type = t }, toks, len);
}

static void pushi(unsigned long i, struct token **toks, size_t *len)
{
    push((struct token) { .type = T_INTLIT, .v.ival = i }, toks, len);
}

static size_t pushasm(const char *str, struct token **toks, size_t *len)
{
    const char *str1 = str;

    while (*str++ != '{');    
    while (*str++ != '\n');

    char asmbuf[1024]; // TODO: dynamic
    char *asmptr = asmbuf;

    while (*str != '}') *asmptr++ = *str++;
    *asmptr = 0;
    str++;

    push((struct token) { .type = T_ASM, .v.sval = strdup(asmbuf) }, toks, len);

    return str - str1;
}

static void push_keyword(const char *str, struct token **toks, size_t *len)
{
    int type = 0; // Will not fail, this function is preceded by a check to iskeyword()

    for (size_t i = 0; i < ARRLEN(keywords); i++)
        if (!strcmp(keywords[i].str, str)) type = keywords[i].token;

    pushnv(type, toks, len);
}

static void push_ident(const char *str, struct token **toks, size_t *len)
{
    push((struct token) { .type = T_IDENT, .v.sval = strdup(str) }, toks, len);
}

static size_t push_strlit(const char *str, struct token **toks, size_t *len)
{
    const char *str2 = str;

    char buf[128];
    char *bufptr = buf;
    str++;

    // TODO: escape characters
    while (*str != '"') *bufptr++ = *str++;
    *bufptr = 0;
    str++;

    push((struct token) { .type = T_STRLIT, .v.sval = strdup(buf) }, toks, len);

    return str - str2;
}



int tokenize(const char *str, struct token **toks)
{
    s_lexer.currline = 1;
    s_lexer.currcol = 0;

    size_t i = 0;
    while (*str)
    {
        switch (*str)
        {
            case '+':
            {
                if (*(++str) == '+')
                {
                    pushnv(T_INC, toks, &i);
                    str++;
                }
                else pushnv(T_PLUS, toks, &i);
                continue;
            }
            
            case '-':
            {
                switch (*(++str))
                {
                    case '-': pushnv(T_DEC, toks, &i); str++; break;
                    case '>': pushnv(T_ARROW, toks, &i); str++; break;
                    default:  pushnv(T_MINUS, toks, &i);
                }
                continue;
            }

            case '*': pushnv(T_STAR,   toks, &i); str++; continue;
            case ';': pushnv(T_SEMI,   toks, &i); str++; continue;
            case '(': pushnv(T_LPAREN, toks, &i); str++; continue;
            case ')': pushnv(T_RPAREN, toks, &i); str++; continue;
            case '[': pushnv(T_LBRACK, toks, &i); str++; continue;
            case ']': pushnv(T_RBRACK, toks, &i); str++; continue;
            case '{': pushnv(T_LBRACE, toks, &i); str++; continue;
            case '}': pushnv(T_RBRACE, toks, &i); str++; continue;
            case ',': pushnv(T_COMMA,  toks, &i); str++; continue;
            case ':': pushnv(T_COLON,  toks, &i); str++; continue;

            case '/':
            {
                char c = *(++str);
                if (c == '/')
                {
                    while (*str != '\n' && *str) str++;
                    str++;
                }
                else if (c == '*')
                {
                    int nest = 1;
                    while (1)
                    {
                        if (*str == '/' && *(str + 1) == '*') nest++;
                        if (*str == '*' && *(str + 1) == '/') nest--;
                        if (!nest) break;
                        str++;
                    }
                    str += 2;
                }
                else pushnv(T_SLASH, toks, &i);
                continue;
            }

            case '!':
                if (*(++str) == '=')
                {
                    pushnv(T_NEQ, toks, &i);
                    str++;
                }
                else pushnv(T_NOT, toks, &i);
                continue;

            case '=':
                if (*(++str) == '=')
                {
                    pushnv(T_EQEQ, toks, &i);
                    str++;
                }
                else pushnv(T_EQ, toks, &i);
                continue;

            case '>':
                if (*(++str) == '=')
                {
                    pushnv(T_GTE, toks, &i);
                    str++;
                }
                else pushnv(T_GT, toks, &i);
                continue;
            case '<':
                if (*(++str) == '=')
                {
                    pushnv(T_LTE, toks, &i);
                    str++;
                }
                else pushnv(T_LT, toks, &i);
                continue;

            case '"':
                str += push_strlit(str, toks, &i);
                continue;

            case '\'':
                str++;
                pushi(*str, toks, &i);
                str += 2;
                continue;

            case '&':
                if (*(++str) == '&')
                {
                    pushnv(T_LAND, toks, &i);
                    str++;
                }
                else pushnv(T_AMP, toks, &i);
                continue;
            
            case '|':
                if (*(++str) == '|')
                {
                    pushnv(T_LOR, toks, &i);
                    str++;
                }
                else pushnv(T_BITOR, toks, &i);
                continue;
        }

        if (isspace(*str))
        {
            switch (*str)
            {
                case ' ':  s_lexer.currcol++; break;
                case '\t': s_lexer.currcol += 4; break;
                case '\n': s_lexer.currline++; break;
            }
            str++;
        }
        else if (isdigit(*str))
            pushi(strtol(str, (char**)&str, 10), toks, &i);
        else if (isalpha(*str) || *str == '_')
        {
            char ident[64];
            size_t j;
            for (j = 0; isidentc(*str) && j < 63; j++, str++) ident[j] = *str;
            ident[j] = 0;

            if (!strcmp(ident, "asm"))
                str += pushasm(str, toks, &i);
            else if (iskeyword(ident))
                push_keyword(ident, toks, &i);
            else
                push_ident(ident, toks, &i);
        }
        else
        {
            printf("Invalid token: '%c'\n", *str);
            abort();
        }
    }

    pushnv(T_EOF, toks, &i);

    return 0;
}
