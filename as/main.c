#define extern_
#include "decl.h"
#include "lib.h"
#include "asm.h"
#include "sym.h"

#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>
#include <string.h>

static char *inf_name = NULL;
static char *outf_name = NULL;

void usage()
{
    fprintf(stderr, "usage: as <input> -o <output>\n");
    exit(-1);
}

void do_cmd_args(int argc, char **argv)
{
    char opt;
    while ((opt = getopt(argc, argv, "o:")) != -1)
    {
        switch (opt)
        {
            case 'o':
                outf_name = strdup(optarg);
                break;

            default: usage(); break;
        }
    }

    for (char **arg = argv + 1; *arg; arg++)
    {
        if ((*arg)[0] == '-' && (*arg)[1] == 'o') arg++;
        else
        {
            inf_name = strdup(*arg);
            break;
        }
    }

    if (!inf_name)
        usage();

    if (!outf_name)
    {
        outf_name = strndup(inf_name, strrchr(inf_name, '.') - inf_name + 2);
        outf_name[strlen(outf_name) - 1] = 'o';
    }
}

void cleanup()
{
    if (g_inf) fclose(g_inf);
    if (g_outf) fclose(g_outf);

    g_inf = NULL; g_outf = NULL;
}

int main(int argc, char **argv)
{
    do_cmd_args(argc, argv);
    
    g_inf = xfopen(inf_name, "r");
    g_outf = xfopen(outf_name, "w+");

    atexit(cleanup);

    printf("as %s -o %s\n", inf_name, outf_name);

    struct symbol sym = {
        .name = inf_name,
        .type = SYMT_FILE,
    };
    addsym(&sym);

    collect_syms();
    assemble();

    cleanup();

    // TODO: TEMP
    char cmd[64];
    snprintf(cmd, 64, "gcc %s -static -fpie", outf_name);
    printf("%s\n", cmd);

    system(cmd);

    return 0;
}
