#include "link.h"
#include "decl.h"

#include <elf.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

static size_t s_base;
static Elf64_Ehdr s_ehdr;

static void get_section(unsigned int n, Elf64_Shdr *shdr)
{
    fseek(g_inf, s_ehdr.e_shoff + n * sizeof(Elf64_Shdr), SEEK_SET);
    fread(shdr, sizeof(Elf64_Shdr), 1, g_inf);
}

static void get_symbol(Elf64_Shdr *symtab, unsigned int n, Elf64_Sym *sym)
{
    fseek(g_inf, symtab->sh_offset + n * sizeof(Elf64_Sym), SEEK_SET);
    fread(sym, sizeof(Elf64_Sym), 1, g_inf);
}

// Copies the raw bytes of a section into output file
static void copy_section(Elf64_Shdr *shdr)
{
    fseek(g_inf, shdr->sh_offset, SEEK_SET);

    uint8_t *buf = malloc(shdr->sh_size);

    fread(buf, shdr->sh_size, 1, g_inf);
    fwrite(buf, shdr->sh_size, 1, g_outf);

    free(buf);
}

// Computes and writes all relocations in an SHT_RELA section
static void do_relocs(Elf64_Shdr *sect, Elf64_Shdr *rel)
{
    for (size_t i = 0; i < rel->sh_size / sizeof(Elf64_Rela); i++)
    {
        fseek(g_outf, sect->sh_offset - sizeof(Elf64_Ehdr), SEEK_SET);

        fseek(g_inf, rel->sh_offset + i * sizeof(Elf64_Rela), SEEK_SET);
        
        Elf64_Rela rela;
        fread(&rela, sizeof(Elf64_Rela), 1, g_inf);

        // Find the symbol
        Elf64_Shdr symtab;
        get_section(rel->sh_link, &symtab);

        Elf64_Sym sym;
        get_symbol(&symtab, ELF64_R_SYM(rela.r_info), &sym);

        // Find the symbol's section
        Elf64_Shdr symsect;
        get_section(sym.st_shndx, &symsect);

        // TODO: keep track of base address of sections within binary file - this will not work
        size_t sbase = symsect.sh_offset - sizeof(Elf64_Ehdr);

        // Seek to relocation offset
        fseek(g_outf, rela.r_offset, SEEK_CUR);

        // Write the computed value
        switch (ELF64_R_TYPE(rela.r_info))
        {
            case R_X86_64_32S:
            {
                uint32_t v = sbase + sym.st_value + s_base + rela.r_addend;
                fwrite(&v, sizeof(uint32_t), 1, g_outf);
                break;
            }
        }
    }

    // TODO: again, will not work
    fseek(g_outf, sect->sh_offset - sizeof(Elf64_Ehdr) + sect->sh_size, SEEK_SET);
}

void link_binary(uint64_t base)
{
    s_base = base;

    fread(&s_ehdr, sizeof(Elf64_Ehdr), 1, g_inf);

    fseek(g_inf, s_ehdr.e_shoff, SEEK_SET);

    for (unsigned int i = 0; i < s_ehdr.e_shnum; i++)
    {
        Elf64_Shdr shdr;
        get_section(i, &shdr);

        if (shdr.sh_type == SHT_PROGBITS)
            copy_section(&shdr);
        else if (shdr.sh_type == SHT_RELA)
        {
            Elf64_Shdr sect;
            get_section(shdr.sh_info, &sect);
            do_relocs(&sect, &shdr);
        }
    }
}
