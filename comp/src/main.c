#include "preproc.h"
#include "lexer.h"
#include "parser.h"
#include "ast.h"
#include "gen.h"
#include "util.h"

#define extern_
#include "decl.h"

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>

const char *infile = NULL, *outfile = NULL;

char *readfile(FILE *f)
{
    fseek(f, 0, SEEK_END);
    
    size_t len = ftell(f);
    char *buf = malloc(len + 1);

    fseek(f, 0, SEEK_SET);

    fread(buf, 1, len, f);
    buf[len] = 0;

    fclose(f);
    return buf;
}

int main(int argc, char **argv)
{
    outfile = "out.s";

    char opt;
    while ((opt = getopt(argc, argv, "o:s:")) != -1)
    {
        switch (opt)
        {
            case 'o':
                outfile = strdup(optarg);
                break;
            case 's':
                infile = strdup(optarg);
                break;
            default:
                printf("Invalid option '%c'\n", opt);
                return -1;
        }
    }

    if (!infile)
    {
        printf("No input file specified!\n");
        return -1;
    }

    g_inf = fopen(infile, "r");
    if (!g_inf)
    {
        perror("Error");
        return -1;
    }

    char *code = readfile(g_inf);    
    char *preproc = preprocess(code, infile);

    tokenize(preproc);

    parse();

    g_outf = fopen(outfile, "w+");
    if (!g_outf)
    {
        perror("Error");
        return -1;
    }

    gen_ast();
    fclose(g_outf);

    system("gcc -g -static out.s");

    return 0;
}
