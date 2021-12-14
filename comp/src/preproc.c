#include <preproc.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

static char *s_input;

char *preprocess(const char *input)
{
    s_input = strdup(input);

    char *output = strdup(input);

    int line = 1;
    char *token = strtok(s_input, "\n");
    while (token)
    {
        if (token[0] == '#')
        {
            size_t linelen = strlen(token);
            size_t start = token - s_input;
            size_t end = start + linelen;

            char *second = strchr(token, ' ');
            *second++ = 0;
            
            token++;
            if (!strcmp(token, "include"))
            {
                char *path = second + 1;
                *strchr(path, '"') = 0;

                FILE *file = fopen(path, "r");
                if (!file)
                {
                    printf("\033[1;31merror: \033[37mat line %d: \033[22m", line);
                    printf("Could not find file '%s'\n", path);
                    exit(-1);
                }

                fseek(file, 0, SEEK_END);
                size_t len = ftell(file);
                fseek(file, 0, SEEK_SET);

                char *contents = malloc(len + 1);
                fread(contents, 1, len, file);
                contents[len] = 0;

                fclose(file);

                output = realloc(output, strlen(output) + len + 1);
                memmove(output + len, output + end, strlen(input) - start);
                memcpy(output + start, contents, len);

                free(contents);
            }
        }
        
        token = strtok(NULL, "\n");
        line++;
    }

    return output;
}