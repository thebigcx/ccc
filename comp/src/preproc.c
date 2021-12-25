#include <preproc.h>
#include <util.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

struct define
{
    const char *name, *val;
};

static struct define *s_defines = NULL;
static unsigned int  s_definecnt = 0;

char *preprocess(const char *code, const char *infilename)
{
    FILE *output = tmpfile();
    char *input = strdup(code);

    for (char *cpy = input; *cpy; cpy++)
    {
        if (*cpy == '/' && *(cpy + 1) == '*')
        {
            while (*cpy != '*' || *(cpy + 1) != '/')
            {
                if (*cpy != '\n') *cpy = ' ';
                cpy++;
            }
            *cpy++ = ' ';
            *cpy   = ' ';
        }

        if (*cpy == '/' && *(cpy + 1) == '/')
        {
            while (*cpy && *cpy != '\n') *cpy++ = ' ';
        }
    }

    int line = 1;
    int discard = 0; // Whether to discard lines
    int nested  = 0; // Nested ifdefs
    char *token;

    fprintf(output, "# 1 \"%s\"\n", infilename);
    while ((token = strsep(&input, "\n")))
    {
        if (token[0] == '#')
        {
            if (!strcmp(++token, "endif"))
            {
                if (!nested)
                {
                    printf("\033[1;31merror: \033[37mfile '%s' at line %d: \033[22m", infilename, line);
                    printf("Unexpected 'endif' directive\n");
                    exit(-1);
                }

                nested--;
                fprintf(output, "\n");
                goto next_line;
            }
            if (discard)
            {
                fprintf(output, "\n");
                goto next_line;
            }

            *strchr(token, ' ') = 0;
            
            if (!strcmp(token, "include"))
            {
                token += strlen(token) + 1; // space and quote
                *strchr(++token, '"') = 0;
            
                FILE *file = fopen(token, "r");
                if (!file)
                {
                    printf("\033[1;31merror: \033[37mfile '%s' at line %d: \033[22m", infilename, line);
                    printf("Could not find file '%s'\n", token);
                    exit(-1);
                }

                char *contents = readfile(file);
                char *preproc  = preprocess(contents, token);
                fprintf(output, "# 1 \"%s\"\n", token);
                fprintf(output, "%s%c\n", preproc, (char)-1);
                fprintf(output, "# %d \"%s\"\n", line + 1, infilename);
            }
            else if (!strcmp(token, "define"))
            {
                char *ident = token += strlen(token) + 1; // space
                
                char *end = strchr(ident, ' ');
                if (end)
                {
                    *end = 0;
                    token = end + 1;
                }
                else token = NULL;

                s_defines = realloc(s_defines, (s_definecnt + 1) * sizeof(struct define));
                s_defines[s_definecnt++] = (struct define)
                {
                    .name = strdup(ident),
                    .val  = token ? strdup(token) : NULL
                };
                fprintf(output, "\n");
            }
            else if (!strcmp(token, "ifdef") || !strcmp(token, "ifndef"))
            {
                int ifndef = !strcmp(token, "ifndef");
                char *ident = token += strlen(token) + 1; // space

                int defined = 0;
                for (unsigned int i = 0; i < s_definecnt; i++)
                {
                    if (!strcmp(s_defines[i].name, ident))
                    {
                        defined = 1;
                        break;
                    }
                }

                if ((ifndef && defined) || (!ifndef && !defined))
                    discard++;
                fprintf(output, "\n");
                nested++;
            }
        }
        else
        {
            if (discard)
                fprintf(output, "\n");
            else
                fprintf(output, "%s\n", token);
        }

next_line:
        line++;
    }

    if (nested)
    {
        printf("\033[1;31merror: \033[37mfile '%s' at line %d: \033[22m", infilename, line - 1);
        printf("No matching 'endif' directive\n");
        exit(-1);
    }

    return readfile(output);
}