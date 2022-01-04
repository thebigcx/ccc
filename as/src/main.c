#include <lexer.h>
#include <asm.h>

#include <stdio.h>

#define TRY(fn) if (fn) { return -1; }

void usage()
{
    printf("usage: as: <input>\n");
}

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        usage();
        return -1;
    }

    FILE *in = fopen(argv[1], "r");
    if (!in)
    {
        char buf[64];
        snprintf(buf, 64, "as: %s", argv[1]);
        perror(buf);
        return -1;
    }

    struct token *toks;
    TRY(lexfile(in, &toks));

    FILE *out = fopen("out.bin", "w+");
    TRY(assemble(out, toks));

    fclose(in);
    fclose(out);
    return 0;
}
