#include <stdio.h>
#include <stdlib.h>

#include <lexer.h>
#include <parser.h>
#include <gen.h>

int main(int argc, char **argv)
{
    (void)argc;

    FILE *f = fopen(argv[1], "r");
    
    fseek(f, 0, SEEK_END);
    
    size_t len = ftell(f);
    char *code = malloc(len + 1);

    fseek(f, 0, SEEK_SET);

    fread(code, 1, len, f);
    code[len] = 0;

    struct token *toks = NULL;
    tokenize(code, &toks);

    struct ast ast;
    parse(toks, &ast);

    FILE *out = fopen("out.s", "w+");
    gen_code(&ast, out);
    fclose(out);

    return 0;
}
