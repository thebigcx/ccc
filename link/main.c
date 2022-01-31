#define extern_
#include "decl.h"
#include "link.h"

#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <elf.h>

static char *inf_name = NULL;
static char *outf_name = NULL;

static int isbin = 0;
static size_t base = 0;

//static Elf64_Ehdr s_ehdr = { 0 };

void usage()
{
    printf("usage: link <object> -o <output>\n");
    exit(-1);
}

void parse_cmd_opts(int argc, char **argv)
{
    char opt;
    while ((opt = getopt(argc, argv, "o:s:b")) != -1)
    {
        switch (opt)
        {
            case 'o':
                outf_name = strdup(optarg);
                break;
            case 's':
                base = strtoul(optarg + 2, NULL, 16);
                break;
            case 'b':
                isbin = 1;
                break;
        }
    }

    if (!argv[optind])
        usage();

    inf_name = argv[optind];

    if (!outf_name)
        outf_name = strdup("a.out");
}

void cleanup()
{
    if (g_inf)  fclose(g_inf);
    if (g_outf) fclose(g_outf);
}

int main(int argc, char **argv)
{
    parse_cmd_opts(argc, argv);

    atexit(cleanup);

    g_inf = fopen(inf_name, "r");
    if (!g_inf)
    {
        printf("link: %s: %s\n", inf_name, strerror(errno));
        return -1;
    }

    g_outf = fopen(outf_name, "w+");
    if (!g_outf)
    {
        printf("link: %s: %s\n", outf_name, strerror(errno));
        return -1;
    }

    if (isbin)
        link_binary(base);
    else
        link_elf();

    /*unsigned char ident[EI_NIDENT] = {
        [EI_MAG0]       = ELFMAG0,
        [EI_MAG1]       = ELFMAG1,
        [EI_MAG2]       = ELFMAG2,
        [EI_MAG3]       = ELFMAG3,
        [EI_CLASS]      = ELFCLASS64,
        [EI_DATA]       = ELFDATA2LSB,
        [EI_VERSION]    = EV_CURRENT,
        [EI_OSABI]      = ELFOSABI_LINUX,
        [EI_ABIVERSION] = 0
    };
    memcpy(&s_ehdr.e_ident, ident, sizeof(ident));
    s_ehdr.e_type      = ET_EXEC;
    s_ehdr.e_machine   = EM_X86_64;
    s_ehdr.e_version   = EV_CURRENT;
    s_ehdr.e_ehsize    = sizeof(Elf64_Ehdr);
    s_ehdr.e_shentsize = sizeof(Elf64_Shdr);
    s_ehdr.e_phnum     = 1;
    s_ehdr.e_phentsize = sizeof(Elf64_Phdr);
    s_ehdr.e_phoff     = sizeof(Elf64_Ehdr);

    fwrite(&s_ehdr, sizeof(Elf64_Ehdr), 1, g_outf);   

    Elf64_Phdr phdr = {
        .p_type = PT_LOAD,
        .p_vaddr = 0x40000,
        .p_flags = PF_X | PF_R | PF_W
    };

    fwrite(&phdr, sizeof(Elf64_Phdr), 1, g_outf);*/

    return 0;
}
