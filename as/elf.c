#include "elf.h"
#include "decl.h"
#include "sym.h"
#include "lib.h"

#include <elf.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>

static Elf64_Ehdr s_ehdr = { 0 }; // ELF header

// Write as much of Ehdr as possible
void elf_begin_file()
{
    unsigned char ident[EI_NIDENT] = {
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
    s_ehdr.e_type      = ET_REL;
    s_ehdr.e_machine   = EM_X86_64;
    s_ehdr.e_version   = EV_CURRENT;
    s_ehdr.e_ehsize    = sizeof(Elf64_Ehdr);
    s_ehdr.e_shentsize = sizeof(Elf64_Shdr);

    fwrite(&s_ehdr, sizeof(Elf64_Ehdr), 1, g_outf);
}

static void write_symbol(struct symbol *sym)
{
    uint8_t type = sym->flags & SYM_SECT ? STT_SECTION : STT_NOTYPE;
    uint8_t bind = sym->flags & SYM_GLOB ? STB_GLOBAL : STB_LOCAL;

    Elf64_Sym esym = {
        .st_name = sym->namei,
        .st_info = ELF64_ST_INFO(bind, type)
    };

    if (!(sym->flags & SYM_UNDEF))
    {
        size_t sect = 1;
        for (struct section *cand = g_sects; cand && cand != sym->sect; cand = cand->next, sect++);
       
        esym.st_value = sym->val;
        esym.st_shndx = sect;
    }

    fwrite(&esym, sizeof(Elf64_Sym), 1, g_outf);
}

static void write_section(struct section *sect)
{
    Elf64_Shdr shdr = { 0 };
    if (sect)
    {
        shdr = (Elf64_Shdr) {
            .sh_offset = sect->offset,
            .sh_size = sect->size,
            .sh_name = sect->namei
        };

        if (!strcmp(sect->name, ".text")) { shdr.sh_type = SHT_PROGBITS; shdr.sh_flags = SHF_ALLOC | SHF_EXECINSTR; }
        else if (!strcmp(sect->name, ".data")) { shdr.sh_type = SHT_PROGBITS; shdr.sh_flags = SHF_ALLOC | SHF_WRITE; }
        else if (!strcmp(sect->name, ".rodata")) { shdr.sh_type = SHT_PROGBITS; shdr.sh_flags = SHF_ALLOC; }
        else if (!strcmp(sect->name, ".bss")) { shdr.sh_type = SHT_NOBITS; shdr.sh_flags = SHF_ALLOC | SHF_WRITE; }
        else if (!strcmp(sect->name, ".strtab")) { shdr.sh_type = SHT_STRTAB; }
        else if (!strcmp(sect->name, ".symtab"))
        {
            size_t lastloc = 1, i = 0;
            for (struct symbol *sym = g_syms; sym; sym = sym->next, i++)
                if (sym->flags & SYM_GLOB) { lastloc = i + 1; break; }

            shdr.sh_type = SHT_SYMTAB;
            shdr.sh_info = lastloc;
            shdr.sh_link = sectnum(sect);
            shdr.sh_entsize = sizeof(Elf64_Sym);
        }
        else if (!strcmp(sect->name, ".shstrtab")) { shdr.sh_type = SHT_STRTAB; }
        else if (!strncmp(sect->name, ".rela", 5))
        {
            shdr.sh_flags = SHF_INFO_LINK,
            shdr.sh_type = SHT_RELA,
            shdr.sh_info = sectnum(sect),
            shdr.sh_entsize = sizeof(Elf64_Rela),
            shdr.sh_link = sectcnt() - 1; // TODO: TEMP
        }
    }

    fwrite(&shdr, sizeof(Elf64_Shdr), 1, g_outf);
}

// Append mandatory sections (shstr, symtab, etc), write section headers, patch Ehdr
void elf_end_file()
{
    sort_symbols();

    struct section *relsects[32];
    size_t relsectcnt = 0;

    for (struct section *s = g_sects; s; s = s->next)
    {
        if (s->rels)
        {
            // Prepend '.rela' to section name
            char *name = strcat(strcpy(malloc(strlen(s->name) + 6), ".rela"), s->name);
            struct section *rel = creatsect(name);
            relsects[relsectcnt++] = rel;

            //rel->next = s->next;
            //s->next = rel;
            
            rel->offset = ftell(g_outf);

            for (struct reloc *r = s->rels; r; r = r->next)
            {
                struct symbol *sym = r->sym->flags & SYM_UNDEF ? r->sym : findsym(r->sym->sect->name);

                uint32_t symidx = 1;
                for (struct symbol *s = g_syms; s; s = s->next, symidx++)
                    if (s == sym) break;

                Elf64_Rela rela = {
                    .r_offset = r->offset,
                    .r_info = ELF64_R_INFO(symidx, r->sym->flags & SYM_UNDEF ? R_X86_64_PLT32 : R_X86_64_32S),
                    .r_addend = r->addend,
                };

                fwrite(&rela, sizeof(Elf64_Rela), 1, g_outf);
            }

            rel->size = ftell(g_outf) - rel->offset;
        }
    }

    for (size_t i = 0; i < relsectcnt; i++)
    {
        struct section *s = findsect(relsects[i]->name + 5);
        relsects[i]->next = s->next;
        s->next = relsects[i];
    }

    struct section *strtab = addsect(".strtab");
    strtab->offset = ftell(g_outf);

    fputc(0, g_outf);
    for (struct symbol *sym = g_syms; sym; sym = sym->next)
    {
        sym->namei = ftell(g_outf) - strtab->offset;
        fwritestr(sym->name, g_outf);
    }

    strtab->size = ftell(g_outf) - strtab->offset;

    struct section *symtab = addsect(".symtab");
    symtab->offset = ftell(g_outf);

    Elf64_Sym esym = { 0 };
    fwrite(&esym, sizeof(Elf64_Sym), 1, g_outf);

    for (struct symbol *sym = g_syms; sym; sym = sym->next)
        write_symbol(sym);

    symtab->size = ftell(g_outf) - symtab->offset;

    struct section *shstrtab = addsect(".shstrtab");
    shstrtab->offset = ftell(g_outf);

    fputc(0, g_outf);
    for (struct section *s = g_sects; s; s = s->next)
    {
        s->namei = ftell(g_outf) - shstrtab->offset;
        fwritestr(s->name, g_outf);
    }

    shstrtab->size = ftell(g_outf) - shstrtab->offset;

    uint64_t shoff = ftell(g_outf);

    write_section(NULL);

    for (struct section *s = g_sects; s; s = s->next)
        write_section(s);

    s_ehdr.e_shoff = shoff;
    s_ehdr.e_shnum = sectcnt() + 1;
    s_ehdr.e_shstrndx = sectnum(shstrtab) + 1;

    fseek(g_outf, 0, SEEK_SET);
    fwrite(&s_ehdr, sizeof(Elf64_Ehdr), 1, g_outf);
}

