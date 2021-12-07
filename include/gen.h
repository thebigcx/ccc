#pragma once

#include <parser.h>
#include <stdio.h>

int gen_code(struct ast *ast, FILE *file);
void gen_ast(struct ast *ast, FILE *file);