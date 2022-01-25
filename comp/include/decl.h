#pragma once

#include <stdio.h>

#ifndef extern_
#define extern_ extern
#endif

struct token;
struct ast;

extern_ FILE *g_inf;
extern_ FILE *g_outf;
extern_ struct token *g_toks;
extern_ struct ast *g_ast;
