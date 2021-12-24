#include <preproc.h>
#include <util.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

char *preprocess(const char *code, const char *infilename)
{
    FILE *output = tmpfile();
    char *input = strdup(code);

    int line = 1;
    char *token;
    
    fprintf(output, "# 1 \"%s\"\n", infilename);
    while ((token = strsep(&input, "\n")))
    {
        if (token[0] == '#')
        {
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
                fprintf(output, "# %d \"%s\"\n", line, infilename);
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