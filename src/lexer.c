#include <lexer.h>
#include <stddef.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <util.h>

struct keyword
{
    const char *str;
    int token;
};

static struct keyword keywords[] =
{
    { "int32", T_INT32 }
};

// Tests whether 'c' is a valid character in an indentifier - letters, numbers, or '_'
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

int tokenize(const char *str, struct token **toks)
{
    size_t i = 0;
    while (*str)
    {
        switch (*str)
        {
            case '+': pushnv(T_PLUS,   toks, &i); str++; continue;
            case '-': pushnv(T_MINUS,  toks, &i); str++; continue;
            case '*': pushnv(T_STAR,   toks, &i); str++; continue;
            case '/': pushnv(T_SLASH,  toks, &i); str++; continue;
            case ';': pushnv(T_SEMI,   toks, &i); str++; continue;
            case '(': pushnv(T_LPAREN, toks, &i); str++; continue;
            case ')': pushnv(T_RPAREN, toks, &i); str++; continue;
            case '[': pushnv(T_LBRACK, toks, &i); str++; continue;
            case ']': pushnv(T_RBRACK, toks, &i); str++; continue;
            case '{': pushnv(T_LBRACE, toks, &i); str++; continue;
            case '}': pushnv(T_RBRACE, toks, &i); str++; continue;
        }

        if (isspace(*str))
            str++;
        else if (isdigit(*str))
            pushi(strtol(str, (char**)&str, 10), toks, &i);
        else if (isalpha(*str) || *str == '_')
        {
            char ident[64];
            size_t j;
            for (j = 0; isidentc(*str) && j < 63; j++, str++) ident[j] = *str;
            ident[j] = 0;

            if (iskeyword(ident))
                push_keyword(ident, toks, &i);
            else
                push_ident(ident, toks, &i);

            str++;
        }
    }

    pushnv(T_EOF, toks, &i);

    return 0;
}
