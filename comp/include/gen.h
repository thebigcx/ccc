#pragma once

#include <stdio.h>

struct ast;

int gen_code(struct ast *ast, FILE *file);
void gen_ast(struct ast *ast, FILE *file);