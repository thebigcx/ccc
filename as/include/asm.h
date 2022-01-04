#pragma once

#include <stddef.h>
#include <stdio.h>

struct token;

int assemble(FILE *out, struct token *toks);
