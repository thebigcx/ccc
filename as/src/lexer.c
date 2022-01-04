#include <lexer.h>

#include <stdlib.h>
#include <ctype.h>
#include <string.h>

static FILE *s_file = NULL;
static struct token **s_toks = NULL;
static size_t s_tokcnt = 0;

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
        switch (c)
        {
            case ':': pushtok(T_COLON, NULL, 0); continue;
        }

        char str[32];
        int strl = 0;
        if (isalnum(c) || c == '_')
        {
            int dig = isdigit(c);

            do str[strl++] = c;
            while ((c = fgetc(file)) != EOF && isdigit(c));
            
            str[strl] = 0;
            ungetc(c, file);

            if (dig)
            {
                unsigned long v = strtoull(str, NULL, 10);
                pushtok(T_IMM, NULL, v);
            }
            else
            {
                pushtok(T_LBL, strdup(str), 0);
            }
        }
    }

    pushtok(T_EOF, NULL, 0);
    return 0;
}
