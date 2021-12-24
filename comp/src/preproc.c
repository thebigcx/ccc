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
    char *token;

    fprintf(output, "# 1 \"%s\"\n", infilename);
    while ((token = strsep(&input, "\n")))
    {
        if (token[0] == '#')
        {
            if (!strcmp(token, "endif"))
            {
                continue;                
            }

            *strchr(++token, ' ') = 0;
            
            if (!strcmp(token, "include"))
            {
            
                token += strlen(token) + 1; // space and quote
                *strchr(++token, '"') = 0;
            
                FILE *file = fopen(token, "r");
                if (!file)
                {
                    printf("\033[1;31merror: \033[37mat line %d: \033[22m", line);
                    printf("Could not find file '%s'\n", token);
                    exit(-1);
                }

                char *contents = readfile(file);
                fprintf(output, "# 1 \"%s\"\n", token);
                fprintf(output, "%s%c\n", contents, (char)-1);
                fprintf(output, "# %d \"%s\"\n", ++line, infilename);
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
            }
            else if (!strcmp(token, "ifdef"))
            {
                //char *ident = token += strlen(token) + 1; // space
            }
        }
        else
        {
            fprintf(output, "%s\n", token);
        }
        line++;
    }

    return readfile(output);
}